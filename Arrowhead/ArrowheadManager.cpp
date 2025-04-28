#include "ArrowheadManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

// Logging helper function - replace with your logging framework if needed
void log(const std::string& message) {
    std::cout << message << std::endl;
}

ArrowheadManager::ArrowheadManager(
    const std::string& systemName, 
    const std::string& systemAddress, 
    int systemPort,
    const std::string& macAddress,
    const std::vector<std::pair<std::string, std::string>>& provided_services,
    const std::vector<std::string>& consumed_services)
    : systemName(systemName), 
      systemAddress(systemAddress), 
      systemPort(systemPort), 
      macAddress(macAddress),
      provided_services(provided_services), 
      consumed_services(consumed_services) 
{
    // Default Arrowhead address settings
    arrowheadAddress = "127.0.0.1";
    serviceRegistryPort = 8443;
    systemRegistryPort = 8437;
    
    // Construct URLs
    setArrowheadServiceRegistry(arrowheadAddress, serviceRegistryPort);
    setArrowheadSystemRegistry(arrowheadAddress, systemRegistryPort);
    
    // Create the HTTP server on the specified port
    server = std::make_unique<ArrowheadServer>(systemPort);
    
    // Register default handlers for all provided services
    for (const auto& service : provided_services) {
        std::string endpoint = service.second;
        
        // Register a default GET handler
        server->on(endpoint, "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = "{}";  // Empty JSON object
            res.prepare_payload();
        });
    }
}

void ArrowheadManager::setArrowheadServiceRegistry(const std::string& address, int port) {
    arrowheadAddress = address;
    serviceRegistryPort = port;
    serviceRegistryURL = "http://" + address + ":" + std::to_string(port) + "/serviceregistry";
}

void ArrowheadManager::setArrowheadSystemRegistry(const std::string& address, int port) {
    arrowheadAddress = address;
    systemRegistryPort = port;
    systemRegistryURL = "http://" + address + ":" + std::to_string(port) + "/systemregistry";
}

void ArrowheadManager::on(const std::string& endpoint, const std::string& method, RequestHandler handler) {
    server->on(endpoint, method, handler);
}

bool ArrowheadManager::startServer() {
    return server->start();
}

void ArrowheadManager::stopServer() {
    server->stop();
}

bool ArrowheadManager::isServerRunning() const {
    return server->isRunning();
}

bool ArrowheadManager::registerSystemAndServices() {
    if (this->provided_services.empty() && this->consumed_services.empty()) {
        log("No services to register or discover.");
        return true;
    }

    static int pingRetries = 1;

    if (!httpPing()) {
        std::string text = "Pinging Arrowhead " + std::to_string(pingRetries) + "...";
        log(text);
        pingRetries += 1;
        this->systemRegistered = false;
        this->servicesRegistered = false;
        this->servicesDiscovered = false;
        return false;
    }
    pingRetries = 1;

    // First, try to register the system
    if (!this->systemRegistered) {
        log("Registering System...");
        this->systemRegistered = registerSystem();
    }

    // Then register services
    if (this->systemRegistered && !this->servicesRegistered) {
        log("Registering Services...");
        this->servicesRegistered = registerServices();
    }

    // Finally discover services
    if (this->systemRegistered && this->servicesRegistered && !this->servicesDiscovered) {
        log("Discovering Services...");
        this->servicesDiscovered = discoverServices();
    }

    // Return true only if all operations were successful
    return this->systemRegistered && this->servicesRegistered && this->servicesDiscovered;
}

bool ArrowheadManager::deregisterMatchingServices() {
    if (this->provided_services.empty()) {
        log("No provided services to deregister.");
        return true;
    }
    
    bool allSuccess = true;
    for (const auto& service : this->provided_services) {
        // De-register service using service definition and URI
        if (!queryAndDeregisterAllServices(service.first, service.second)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

std::string ArrowheadManager::encodeUrl(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

int ArrowheadManager::httpGet(const std::string& url, std::string& response) {
    try {
        // Parse the URL
        std::string host;
        std::string port;
        std::string target;
        std::string protocol;
        
        size_t protocolEnd = url.find("://");
        if (protocolEnd != std::string::npos) {
            protocol = url.substr(0, protocolEnd);
            protocolEnd += 3; // skip over "://"
        } else {
            protocol = "http";
            protocolEnd = 0;
        }
        
        size_t hostEnd = url.find(':', protocolEnd);
        if (hostEnd == std::string::npos) {
            hostEnd = url.find('/', protocolEnd);
            if (hostEnd == std::string::npos) {
                host = url.substr(protocolEnd);
                target = "/";
            } else {
                host = url.substr(protocolEnd, hostEnd - protocolEnd);
                target = url.substr(hostEnd);
            }
            port = (protocol == "https") ? "443" : "80";
        } else {
            host = url.substr(protocolEnd, hostEnd - protocolEnd);
            size_t portEnd = url.find('/', hostEnd);
            if (portEnd == std::string::npos) {
                port = url.substr(hostEnd + 1);
                target = "/";
            } else {
                port = url.substr(hostEnd + 1, portEnd - (hostEnd + 1));
                target = url.substr(portEnd);
            }
        }

        // Set up Boost Beast for HTTP request
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Make the connection
        stream.connect(results);
        
        // Set up an HTTP GET request
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // ===== LOG THE FULL REQUEST =====
        {
            std::ostringstream oss;
            oss << req; // boost::beast::http::request supports operator<<
            log("Outgoing HTTP Request:\n" + oss.str());
        }

        // Send the HTTP request
        http::write(stream, req);
        
        // Buffer for reading
        beast::flat_buffer buffer;
        
        // Response object
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);

        // ===== LOG THE FULL RESPONSE =====
        {
            std::ostringstream oss;
            oss << res; // boost::beast::http::response supports operator<<
            log("Incoming HTTP Response:\n" + oss.str());
        }

        // Get the response body
        response = res.body();
        
        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
            
        return res.result_int();
    }
    catch(std::exception const& e) {
        log("Error during HTTP GET: " + std::string(e.what()));
        return 0;
    }
}


int ArrowheadManager::httpPost(const std::string& url, const std::string& payload, std::string& response) {
    try {
        // Parse the URL
        std::string host;
        std::string port;
        std::string target;
        std::string protocol;
        
        size_t protocolEnd = url.find("://");
        if (protocolEnd != std::string::npos) {
            protocol = url.substr(0, protocolEnd);
            protocolEnd += 3; // skip over "://"
        } else {
            protocol = "http";
            protocolEnd = 0;
        }
        
        size_t hostEnd = url.find(':', protocolEnd);
        if (hostEnd == std::string::npos) {
            hostEnd = url.find('/', protocolEnd);
            if (hostEnd == std::string::npos) {
                host = url.substr(protocolEnd);
                target = "/";
            } else {
                host = url.substr(protocolEnd, hostEnd - protocolEnd);
                target = url.substr(hostEnd);
            }
            port = (protocol == "https") ? "443" : "80";
        } else {
            host = url.substr(protocolEnd, hostEnd - protocolEnd);
            size_t portEnd = url.find('/', hostEnd);
            if (portEnd == std::string::npos) {
                port = url.substr(hostEnd + 1);
                target = "/";
            } else {
                port = url.substr(hostEnd + 1, portEnd - (hostEnd + 1));
                target = url.substr(portEnd);
            }
        }

        // Set up Boost Beast for HTTP request
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Make the connection on the IP address we get from a lookup
        stream.connect(results);
        
        // Set up an HTTP POST request message
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = payload;
        req.prepare_payload();
        
        // Send the HTTP request to the remote host
        http::write(stream, req);
        
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;
        
        // Declare a container to hold the response
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);
        
        // Get the response body
        response = res.body();
        
        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        // not_connected happens sometimes so don't bother reporting it
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
            
        return res.result_int();
    }
    catch(std::exception const& e) {
        log("Error during HTTP POST: " + std::string(e.what()));
        return 0;
    }
}

int ArrowheadManager::httpDelete(const std::string& url, std::string& response) {
    try {
        // Parse the URL
        std::string host;
        std::string port;
        std::string target;
        std::string protocol;
        
        size_t protocolEnd = url.find("://");
        if (protocolEnd != std::string::npos) {
            protocol = url.substr(0, protocolEnd);
            protocolEnd += 3; // skip over "://"
        } else {
            protocol = "http";
            protocolEnd = 0;
        }
        
        size_t hostEnd = url.find(':', protocolEnd);
        if (hostEnd == std::string::npos) {
            hostEnd = url.find('/', protocolEnd);
            if (hostEnd == std::string::npos) {
                host = url.substr(protocolEnd);
                target = "/";
            } else {
                host = url.substr(protocolEnd, hostEnd - protocolEnd);
                target = url.substr(hostEnd);
            }
            port = (protocol == "https") ? "443" : "80";
        } else {
            host = url.substr(protocolEnd, hostEnd - protocolEnd);
            size_t portEnd = url.find('/', hostEnd);
            if (portEnd == std::string::npos) {
                port = url.substr(hostEnd + 1);
                target = "/";
            } else {
                port = url.substr(hostEnd + 1, portEnd - (hostEnd + 1));
                target = url.substr(portEnd);
            }
        }

        // Set up Boost Beast for HTTP request
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Make the connection on the IP address we get from a lookup
        stream.connect(results);
        
        // Set up an HTTP DELETE request message
        http::request<http::string_body> req{http::verb::delete_, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
        // Send the HTTP request to the remote host
        http::write(stream, req);
        
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;
        
        // Declare a container to hold the response
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);
        
        // Get the response body
        response = res.body();
        
        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        // not_connected happens sometimes so don't bother reporting it
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
            
        return res.result_int();
    }
    catch(std::exception const& e) {
        log("Error during HTTP DELETE: " + std::string(e.what()));
        return 0;
    }
}

bool ArrowheadManager::httpPing() {
    std::string response;
    std::string pingUrl = this->serviceRegistryURL + "/echo";
    
    int httpCode = httpGet(pingUrl, response);
    
    if (httpCode == 200) {
        return true;
    } else {
        log("Ping Failed: HTTP Code " + std::to_string(httpCode));
        return false;
    }
}

json ArrowheadManager::createSystemRegistrationJson() {
    json doc;
    
    json provider = json::object();
    provider["address"] = this->systemAddress;
    provider["authenticationInfo"] = "";
    provider["deviceName"] = "Device";
    provider["macAddress"] = this->macAddress;

    json system = json::object();
    system["address"] = this->systemAddress;
    system["port"] = this->systemPort;
    system["systemName"] = this->systemName;

    doc["provider"] = provider;
    doc["system"] = system;
    doc["version"] = 1;
    
    return doc;
}

bool ArrowheadManager::registerSystem() {
    if (!queryAndDeregisterAllSystems()) {
        return false;
    }
    
    std::string registrationUrl = this->systemRegistryURL + "/register";
    json requestJson = createSystemRegistrationJson();
    std::string requestBody = requestJson.dump();
    std::string responseBody;
    
    int httpResponseCode = httpPost(registrationUrl, requestBody, responseBody);
    
    if (httpResponseCode == 201) {
        log("System successfully registered.");
        return true;
    } else if (httpResponseCode == 400) {
        try {
            json errorDoc = json::parse(responseBody);
            std::string errorMessage = errorDoc["errorMessage"];
            if (errorMessage.find("already exists") != std::string::npos) {
                log("System already registered. Treating as success.");
                return true;
            }
        } catch (const std::exception& e) {
            log("Error parsing error response: " + std::string(e.what()));
        }
    }

    log("Failed to register system, HTTP response code: " + std::to_string(httpResponseCode));
    log(responseBody);
    
    return false;
}

json ArrowheadManager::createServiceRegistrationJson(const std::string& serviceDefinition, const std::string& serviceUri) {
    json doc;
    
    doc["serviceDefinition"] = serviceDefinition;
    
    json providerSystem = json::object();
    providerSystem["systemName"] = this->systemName;
    providerSystem["address"] = this->systemAddress;
    providerSystem["port"] = this->systemPort;
    
    doc["providerSystem"] = providerSystem;
    doc["serviceUri"] = serviceUri;
    
    json interfaces = json::array();
    interfaces.push_back("HTTP-INSECURE-JSON");
    doc["interfaces"] = interfaces;
    
    return doc;
}

bool ArrowheadManager::registerServices() {
    if (!deregisterMatchingServices()) {
        return false;
    }
    
    bool allSuccess = true;
    
    for (const auto& service : provided_services) {
        std::string serviceRegistryRegisterURL = this->serviceRegistryURL + "/register";
        json doc = createServiceRegistrationJson(service.first, service.second);
        std::string requestBody = doc.dump();
        std::string responsePayload;
        
        int httpResponseCode = httpPost(serviceRegistryRegisterURL, requestBody, responsePayload);
        
        if (httpResponseCode == 201) {
            log("Service successfully registered.");
            continue;
        } else if (httpResponseCode == 400) {
            try {
                json errorDoc = json::parse(responsePayload);
                std::string errorMessage = errorDoc["errorMessage"];
                if (errorMessage.find("already exists") != std::string::npos) {
                    log("Service already exists. Treating as success.");
                    continue;
                }
            } catch (const std::exception& e) {
                log("Error parsing error response: " + std::string(e.what()));
            }
        }
        
        allSuccess = false;
        log("Failed to register " + service.second + " service, HTTP response code: " + std::to_string(httpResponseCode));
        log(responsePayload);
        log(requestBody);
    }
    
    return allSuccess;
}

bool ArrowheadManager::deregisterService(const std::string& address, const std::string& port, 
                                        const std::string& serviceDefinition, const std::string& serviceUri, 
                                        const std::string& systemName) {
    std::string deregistrationUrl = this->serviceRegistryURL + "/unregister";
    deregistrationUrl += "?address=" + address;
    deregistrationUrl += "&port=" + port;
    deregistrationUrl += "&service_definition=" + serviceDefinition;
    deregistrationUrl += "&service_uri=" + encodeUrl(serviceUri);
    deregistrationUrl += "&system_name=" + systemName;
    
    log("Attempting to deregister service: " + serviceDefinition);
    
    std::string response;
    int httpResponseCode = httpDelete(deregistrationUrl, response);
    
    if (httpResponseCode == 200) {
        log("Service deregistered successfully: " + serviceDefinition);
        return true;
    } else {
        log("Failed to deregister service. HTTP response code: " + std::to_string(httpResponseCode));
        return false;
    }
}

bool ArrowheadManager::queryAndDeregisterAllServices(const std::string& serviceDefinition, const std::string& serviceUri) {
    std::string queryUrl = this->serviceRegistryURL + "/mgmt?direction=ASC&sort_field=id";
    std::string responsePayload;
    
    int httpResponseCode = httpGet(queryUrl, responsePayload);
    bool ret = true;
    
    if (httpResponseCode == 200) {
        log("De-registering " + serviceDefinition);
        
        try {
            json responseDoc = json::parse(responsePayload);
            
            if (!responseDoc.contains("data") || !responseDoc["data"].is_array()) {
                log("Invalid response format");
                return false;
            }
            
            json services = responseDoc["data"];
            
            for (const auto& serviceObj : services) {
                std::string ah_ServiceDefinition;
                std::string ah_ServiceUri;
                
                if (serviceObj.contains("serviceDefinition") && 
                    serviceObj["serviceDefinition"].contains("serviceDefinition")) {
                    ah_ServiceDefinition = serviceObj["serviceDefinition"]["serviceDefinition"];
                } else {
                    continue;
                }
                
                if (serviceObj.contains("serviceUri")) {
                    ah_ServiceUri = serviceObj["serviceUri"];
                } else {
                    continue;
                }
                
                // Check if the current service matches the specified service definition and URI
                // Case-insensitive comparison would be better but requires additional functions
                if (ah_ServiceDefinition == serviceDefinition && ah_ServiceUri == serviceUri) {
                    ret = false;
                    
                    std::string ah_Address = serviceObj["provider"]["address"];
                    std::string ah_Port = std::to_string(serviceObj["provider"]["port"].get<int>());
                    std::string ah_SystemName = serviceObj["provider"]["systemName"];
                    
                    if (deregisterService(ah_Address, ah_Port, ah_ServiceDefinition, ah_ServiceUri, ah_SystemName)) {
                        ret = true;
                    }
                }
            }
        } catch (const std::exception& e) {
            log("Error parsing service query response: " + std::string(e.what()));
            return false;
        }
    } else {
        log("Failed to query services. HTTP response code: " + std::to_string(httpResponseCode));
    }
    
    return ret;
}

bool ArrowheadManager::deregisterSystem(const std::string& systemName, const std::string& address, int port) {
    std::string deregistrationUrl = this->serviceRegistryURL + "/unregister-system";
    deregistrationUrl += "?system_name=" + systemName;
    deregistrationUrl += "&address=" + address;
    deregistrationUrl += "&port=" + std::to_string(port);
    
    log("Attempting to deregister system: " + systemName + " at " + address + ":" + std::to_string(port));
    
    std::string responseBody;
    int httpResponseCode = httpDelete(deregistrationUrl, responseBody);
    
    if (httpResponseCode == 200) {
        log("System deregistered successfully.");
        return true;
    } else {
        log("Failed to deregister system. HTTP response code: " + std::to_string(httpResponseCode));
        log(responseBody);
        return false;
    }
}

bool ArrowheadManager::queryAndDeregisterAllSystems() {
    std::string queryUrl = this->systemRegistryURL + "/mgmt/systems";
    std::string responsePayload;
    
    log("Querying all systems...");
    
    int httpResponseCode = httpGet(queryUrl, responsePayload);
    bool ret = true;
    
    if (httpResponseCode == 200) {
        log("Query successful. Processing systems for deregistration...");
        
        try {
            json responseDoc = json::parse(responsePayload);
            
            if (!responseDoc.contains("data") || !responseDoc["data"].is_array()) {
                log("Invalid response format");
                return false;
            }
            
            json systems = responseDoc["data"];
            
            for (const auto& systemObj : systems) {
                if (!systemObj.contains("system")) {
                    continue;
                }
                
                json system = systemObj["system"];
                std::string queriedSystemName = system["systemName"];
                
                if (queriedSystemName == this->systemName) {
                    std::string address = system["address"];
                    int port = system["port"];
                    
                    // Deregister the system using its address, port, and name
                    if (deregisterSystem(this->systemName, address, port)) {
                        log("Deregistered system: " + this->systemName + " at " + address + ":" + std::to_string(port));
                    } else {
                        log("Failed to deregister system: " + this->systemName + " at " + address + ":" + std::to_string(port));
                        ret = false;
                    }
                }
            }
        } catch (const std::exception& e) {
            log("Error parsing system query response: " + std::string(e.what()));
            return false;
        }
    } else {
        log("Failed to query systems. HTTP response code: " + std::to_string(httpResponseCode));
        ret = false;
    }
    
    return ret;
}

json ArrowheadManager::createServiceQueryJson(const std::string& serviceDefinition) {
    json query;
    query["serviceDefinitionRequirement"] = serviceDefinition;
    
    json interfaceRequirements = json::array();
    interfaceRequirements.push_back("HTTP-INSECURE-JSON");
    query["interfaceRequirements"] = interfaceRequirements;
    
    json securityRequirements = json::array();
    securityRequirements.push_back("NOT_SECURE");
    query["securityRequirements"] = securityRequirements;
    
    return query;
}

bool ArrowheadManager::parseServiceQueryResponse(const std::string& response, 
                                               const std::string& serviceDefinition) {
    try {
        json responseJson = json::parse(response);
        
        if (!responseJson.contains("unfilteredHits")) {
            log("Invalid response format: missing unfilteredHits");
            return false;
        }
        
        if (responseJson["unfilteredHits"] == 0) {
            log("No services found.");
            return false;
        }
        
        if (!responseJson.contains("serviceQueryData") || !responseJson["serviceQueryData"].is_array()) {
            log("Invalid response format: missing serviceQueryData");
            return false;
        }
        
        for (const auto& serviceObject : responseJson["serviceQueryData"]) {
            ServiceInfo info;
            
            log("Discovered Service: ");
            info.serviceName = serviceDefinition;
            log(serviceDefinition);
            
            if (serviceObject.contains("provider")) {
                if (serviceObject["provider"].contains("address")) {
                    info.providerAddress = serviceObject["provider"]["address"];
                    log(info.providerAddress);
                }
                
                if (serviceObject["provider"].contains("port")) {
                    info.providerPort = serviceObject["provider"]["port"];
                    log(std::to_string(info.providerPort));
                }
            }
            
            if (serviceObject.contains("serviceUri")) {
                info.serviceUri = serviceObject["serviceUri"];
                log(info.serviceUri);
            }
            
            this->discovered_services.push_back(info);
        }
        
        return !this->discovered_services.empty();
    } catch (const std::exception& e) {
        log("Error parsing service query response: " + std::string(e.what()));
        return false;
    }
}

bool ArrowheadManager::discoverServices() {
    if (this->consumed_services.empty()) {
        log("No consumed services to discover.");
        return true; // Assuming discovering no services is not an error.
    }
    
    this->discovered_services.clear();
    bool ret = true;
    
    for (const auto& service : this->consumed_services) {
        log("Asking for: " + service + " Service");
        
        std::string serviceRegistryQueryURL = this->serviceRegistryURL + "/query";
        json queryJson = createServiceQueryJson(service);
        std::string queryPayload = queryJson.dump();
        std::string responsePayload;
        
        int httpResponseCode = httpPost(serviceRegistryQueryURL, queryPayload, responsePayload);
        
        if (httpResponseCode == 200) {
            log("200 Got Payload: " + responsePayload);
            
            if (!parseServiceQueryResponse(responsePayload, service)) {
                log("Failed to parse or no services found");
                ret = false;
            }
        } else {
            log("Error during service discovery: " + std::to_string(httpResponseCode));
            log(responsePayload);
            ret = false;
        }
    }
    
    return ret;
}

std::string ArrowheadManager::consumeService(const std::string& serviceName) {
    for (const auto& service : this->discovered_services) {
        if (service.serviceName == serviceName) {
            // Construct the service URL
            std::string serviceUrl = "http://" + service.providerAddress + ":" + 
                                    std::to_string(service.providerPort) + service.serviceUri;
            
            std::string payload;
            int httpResponseCode = httpGet(serviceUrl, payload);
            
            if (httpResponseCode == 200) {
                log("Service response: " + payload);
                return payload; // Return the successful payload
            } else {
                log("Failed to consume " + serviceName + " service, HTTP response code: " + 
                    std::to_string(httpResponseCode));
                log("Attempting to re-discover...");
                this->servicesDiscovered = false;
                return ""; // Service consumption failed
            }
        }
    }
    
    log("Service not found: " + serviceName);
    return ""; // Service name not matched
}

bool ArrowheadManager::consumeService(const std::string& serviceName, 
                                     const std::string& argumentName, 
                                     int argumentValue) {
    for (const auto& service : this->discovered_services) {
        if (service.serviceName == serviceName) {
            // Construct the service URL with the argument name and value as query parameters
            std::string serviceUrl = "http://" + service.providerAddress + ":" + 
                                    std::to_string(service.providerPort) + service.serviceUri + 
                                    "?" + argumentName + "=" + std::to_string(argumentValue);
            
            log(serviceUrl);
            std::string payload;
            int httpResponseCode = httpGet(serviceUrl, payload);
            
            log(payload);
            
            if (httpResponseCode == 200) {
                log("Service response: " + payload);
                
                try {
                    // Parse the JSON response
                    json doc = json::parse(payload);
                    
                    if (doc.contains("responseVal")) {
                        std::string response = doc["responseVal"];
                        // Convert to lowercase for case-insensitive comparison
                        std::transform(response.begin(), response.end(), response.begin(),
                                      [](unsigned char c){ return std::tolower(c); });
                        bool responseVal = (response == "true");
                        
                        return responseVal;
                    }
                } catch (const std::exception& e) {
                    log("JSON parsing failed: " + std::string(e.what()));
                    return false;
                }
                
                return false; // No valid responseVal found
            } else {
                log("Failed to consume " + serviceName + " service, HTTP response code: " + 
                    std::to_string(httpResponseCode));
                log(payload);
                this->servicesDiscovered = false;
                return false; // Service consumption failed
            }
        }
    }
    
    log("Service not found: " + serviceName);
    return false; // Service name not matched
}