#include "ArrowheadManager.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace arrowhead {

using json = nlohmann::json;

// Constructor from configuration
ArrowheadManager::ArrowheadManager(const ArrowheadConfig& config)
    : config_(config)
{
    // Set up HTTP client with logging
    httpClient_.setLogCallback([this](const std::string& message) {
        this->log(message);
    });
    
    // Create the HTTP server
    server_ = std::make_unique<ArrowheadServer>(config_.getSystem().port);
    
    // Register default handlers for all provided services
    for (const auto& service : config_.getProvidedServices()) {
        // Register a default GET handler
        server_->on(service.uri, "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = "{}";  // Empty JSON object
            res.prepare_payload();
        });
    }
}

// Constructor with direct parameters
ArrowheadManager::ArrowheadManager(
    const std::string& systemName, 
    const std::string& systemAddress, 
    int systemPort,
    const std::string& macAddress,
    const std::vector<std::pair<std::string, std::string>>& provided_services,
    const std::vector<std::string>& consumed_services)
{
    // Create the configuration object
    SystemConfig system;
    system.name = systemName;
    system.address = systemAddress;
    system.port = systemPort;
    system.macAddress = macAddress;
    
    // Convert provided services
    std::vector<ServiceConfig> providedServices;
    for (const auto& [definition, uri] : provided_services) {
        ServiceConfig service;
        service.definition = definition;
        service.uri = uri;
        providedServices.push_back(service);
    }
    
    // Set default core systems
    CoreSystemConfig coreSystems;
    coreSystems.serviceRegistry.protocol = "http";
    coreSystems.serviceRegistry.host = "127.0.0.1";
    coreSystems.serviceRegistry.port = 8443;
    coreSystems.serviceRegistry.path = "/serviceregistry";
    
    coreSystems.systemRegistry.protocol = "http";
    coreSystems.systemRegistry.host = "127.0.0.1";
    coreSystems.systemRegistry.port = 8437;
    coreSystems.systemRegistry.path = "/systemregistry";
    
    // Create the configuration
    config_ = ArrowheadConfig(system, providedServices, consumed_services, coreSystems);
    
    // Set up HTTP client with logging
    httpClient_.setLogCallback([this](const std::string& message) {
        this->log(message);
    });
    
    // Create the HTTP server
    server_ = std::make_unique<ArrowheadServer>(system.port);
    
    // Register default handlers for all provided services
    for (const auto& service : providedServices) {
        // Register a default GET handler
        server_->on(service.uri, "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = "{}";  // Empty JSON object
            res.prepare_payload();
        });
    }
}

void ArrowheadManager::setServiceRegistry(const std::string& host, int port) {
    auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    const_cast<EndpointConfig&>(serviceRegistry).host = host;
    const_cast<EndpointConfig&>(serviceRegistry).port = port;
}

void ArrowheadManager::setSystemRegistry(const std::string& host, int port) {
    auto& systemRegistry = config_.getCoreSystems().systemRegistry;
    const_cast<EndpointConfig&>(systemRegistry).host = host;
    const_cast<EndpointConfig&>(systemRegistry).port = port;
}

void ArrowheadManager::on(const std::string& endpoint, const std::string& method, RequestHandler handler) {
    server_->on(endpoint, method, handler);
}

bool ArrowheadManager::startServer() {
    return server_->start();
}

void ArrowheadManager::stopServer() {
    server_->stop();
}

bool ArrowheadManager::isServerRunning() const {
    return server_->isRunning();
}

bool ArrowheadManager::registerSystemAndServices() {
    const auto& providedServices = config_.getProvidedServices();
    const auto& consumedServices = config_.getConsumedServices();
    
    if (providedServices.empty() && consumedServices.empty()) {
        log("No services to register or discover.");
        return true;
    }

    static int pingRetries = 1;

    if (!ping()) {
        std::string text = "Pinging Arrowhead " + std::to_string(pingRetries) + "...";
        log(text);
        pingRetries += 1;
        systemRegistered_ = false;
        servicesRegistered_ = false;
        servicesDiscovered_ = false;
        return false;
    }
    pingRetries = 1;

    // First, try to register the system
    if (!systemRegistered_) {
        log("Registering System...");
        systemRegistered_ = registerSystem();
    }

    // Then register services
    if (systemRegistered_ && !servicesRegistered_) {
        log("Registering Services...");
        servicesRegistered_ = registerServices();
    }

    // Finally discover services
    if (systemRegistered_ && servicesRegistered_ && !servicesDiscovered_) {
        log("Discovering Services...");
        servicesDiscovered_ = discoverServices();
    }

    // Return true only if all operations were successful
    return systemRegistered_ && servicesRegistered_ && servicesDiscovered_;
}

bool ArrowheadManager::ping() {
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    std::string pingUrl = serviceRegistry.getUrl() + "/echo";
    
    HttpResponse response = httpClient_.get(
        serviceRegistry.protocol,
        serviceRegistry.host,
        serviceRegistry.port,
        serviceRegistry.path + "/echo");
    
    if (response.statusCode == 200) {
        return true;
    } else {
        log("Ping Failed: HTTP Code " + std::to_string(response.statusCode));
        return false;
    }
}

json ArrowheadManager::createSystemRegistrationJson() {
    const auto& system = config_.getSystem();
    
    json doc;
    
    json provider = json::object();
    provider["address"] = system.address;
    provider["authenticationInfo"] = "";
    provider["deviceName"] = "Device";
    provider["macAddress"] = system.macAddress;

    json systemJson = json::object();
    systemJson["address"] = system.address;
    systemJson["port"] = system.port;
    systemJson["systemName"] = system.name;

    doc["provider"] = provider;
    doc["system"] = systemJson;
    doc["version"] = 1;
    
    return doc;
}

bool ArrowheadManager::registerSystem() {
    // if (!queryAndDeregisterAllSystems()) {
    //     return false;
    // }
    
    const auto& systemRegistry = config_.getCoreSystems().systemRegistry;
    
    json requestJson = createSystemRegistrationJson();
    std::string requestBody = requestJson.dump();
    
    HttpResponse response = httpClient_.post(
        systemRegistry.protocol,
        systemRegistry.host,
        systemRegistry.port,
        systemRegistry.path + "/register",
        requestBody);
    
    if (response.statusCode == 201) {
        log("System successfully registered.");
        return true;
    } else if (response.statusCode == 400) {
        try {
            json errorDoc = json::parse(response.body);
            std::string errorMessage = errorDoc["errorMessage"];
            if (errorMessage.find("already exists") != std::string::npos) {
                log("System already registered. Treating as success.");
                return true;
            }
        } catch (const std::exception& e) {
            log("Error parsing error response: " + std::string(e.what()));
        }
    }

    log("Failed to register system, HTTP response code: " + std::to_string(response.statusCode));
    log(response.body);
    
    return false;
}

json ArrowheadManager::createServiceRegistrationJson(const std::string& serviceDefinition, const std::string& serviceUri) {
    const auto& system = config_.getSystem();
    
    json doc;
    
    doc["serviceDefinition"] = serviceDefinition;
    
    json providerSystem = json::object();
    providerSystem["systemName"] = system.name;
    providerSystem["address"] = system.address;
    providerSystem["port"] = system.port;
    
    doc["providerSystem"] = providerSystem;
    doc["serviceUri"] = serviceUri;
    
    json interfaces = json::array();
    interfaces.push_back("HTTP-INSECURE-JSON");
    doc["interfaces"] = interfaces;
    
    return doc;
}

bool ArrowheadManager::registerServices() {
    // if (!deregisterMatchingServices()) {
    //     return false;
    // }
    
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    const auto& providedServices = config_.getProvidedServices();
    
    bool allSuccess = true;
    
    for (const auto& service : providedServices) {
        json doc = createServiceRegistrationJson(service.definition, service.uri);
        std::string requestBody = doc.dump();
        
        HttpResponse response = httpClient_.post(
            serviceRegistry.protocol,
            serviceRegistry.host,
            serviceRegistry.port,
            serviceRegistry.path + "/register",
            requestBody);
        
        if (response.statusCode == 201) {
            log("Service successfully registered.");
            continue;
        } else if (response.statusCode == 400) {
            try {
                json errorDoc = json::parse(response.body);
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
        log("Failed to register " + service.uri + " service, HTTP response code: " + std::to_string(response.statusCode));
        log(response.body);
        log(requestBody);
    }
    
    return allSuccess;
}

bool ArrowheadManager::deregisterService(
    const std::string& address, 
    const std::string& port, 
    const std::string& serviceDefinition, 
    const std::string& serviceUri, 
    const std::string& systemName) {
    
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    
    // Construct the query string
    std::string query = "address=" + address +
                        "&port=" + port +
                        "&service_definition=" + serviceDefinition +
                        "&service_uri=" + HttpClient::urlEncode(serviceUri) +
                        "&system_name=" + systemName;
    
    log("Attempting to deregister service: " + serviceDefinition);
    
    HttpResponse response = httpClient_.del(
        serviceRegistry.protocol,
        serviceRegistry.host,
        serviceRegistry.port,
        serviceRegistry.path + "/unregister",
        query);
    
    if (response.statusCode == 200) {
        log("Service deregistered successfully: " + serviceDefinition);
        return true;
    } else {
        log("Failed to deregister service. HTTP response code: " + std::to_string(response.statusCode));
        return false;
    }
}

bool ArrowheadManager::deregisterMatchingServices() {
    const auto& providedServices = config_.getProvidedServices();
    
    if (providedServices.empty()) {
        log("No provided services to deregister.");
        return true;
    }
    
    bool allSuccess = true;
    for (const auto& service : providedServices) {
        if (!queryAndDeregisterAllServices(service.definition, service.uri)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool ArrowheadManager::queryAndDeregisterAllServices(
    const std::string& serviceDefinition, 
    const std::string& serviceUri) {
    
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    
    HttpResponse response = httpClient_.get(
        serviceRegistry.protocol,
        serviceRegistry.host,
        serviceRegistry.port,
        serviceRegistry.path + "/mgmt",
        "direction=ASC&sort_field=id");
    
    bool ret = true;
    
    if (response.statusCode == 200) {
        log("De-registering " + serviceDefinition);
        
        try {
            json responseDoc = json::parse(response.body);
            
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
        log("Failed to query services. HTTP response code: " + std::to_string(response.statusCode));
    }
    
    return ret;
}

bool ArrowheadManager::deregisterSystem(
    const std::string& systemName, 
    const std::string& address, 
    int port) {
    
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    
    // Construct the query string
    std::string query = "system_name=" + systemName +
                        "&address=" + address +
                        "&port=" + std::to_string(port);
    
    log("Attempting to deregister system: " + systemName + " at " + address + ":" + std::to_string(port));
    
    HttpResponse response = httpClient_.del(
        serviceRegistry.protocol,
        serviceRegistry.host,
        serviceRegistry.port,
        serviceRegistry.path + "/unregister-system",
        query);
    
    if (response.statusCode == 200) {
        log("System deregistered successfully.");
        return true;
    } else {
        log("Failed to deregister system. HTTP response code: " + std::to_string(response.statusCode));
        log(response.body);
        return false;
    }
}

bool ArrowheadManager::queryAndDeregisterAllSystems() {
    const auto& systemRegistry = config_.getCoreSystems().systemRegistry;
    const auto& system = config_.getSystem();
    
    log("Querying all systems...");
    
    HttpResponse response = httpClient_.get(
        systemRegistry.protocol,
        systemRegistry.host,
        systemRegistry.port,
        systemRegistry.path + "/mgmt/systems");
    
    bool ret = true;
    
    if (response.statusCode == 200) {
        log("Query successful. Processing systems for deregistration...");
        
        try {
            json responseDoc = json::parse(response.body);
            
            if (!responseDoc.contains("data") || !responseDoc["data"].is_array()) {
                log("Invalid response format");
                return false;
            }
            
            json systems = responseDoc["data"];
            
            for (const auto& systemObj : systems) {
                if (!systemObj.contains("system")) {
                    continue;
                }
                
                json systemJson = systemObj["system"];
                std::string queriedSystemName = systemJson["systemName"];
                
                if (queriedSystemName == system.name) {
                    std::string address = systemJson["address"];
                    int port = systemJson["port"];
                    
                    // Deregister the system using its address, port, and name
                    if (deregisterSystem(system.name, address, port)) {
                        log("Deregistered system: " + system.name + " at " + address + ":" + std::to_string(port));
                    } else {
                        log("Failed to deregister system: " + system.name + " at " + address + ":" + std::to_string(port));
                        ret = false;
                    }
                }
            }
        } catch (const std::exception& e) {
            log("Error parsing system query response: " + std::string(e.what()));
            return false;
        }
    } else {
        log("Failed to query systems. HTTP response code: " + std::to_string(response.statusCode));
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

bool ArrowheadManager::parseServiceQueryResponse(
    const std::string& response, 
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
            
            discoveredServices_.push_back(info);
        }
        
        return !discoveredServices_.empty();
    } catch (const std::exception& e) {
        log("Error parsing service query response: " + std::string(e.what()));
        return false;
    }
}

bool ArrowheadManager::discoverServices() {
    const auto& consumedServices = config_.getConsumedServices();
    const auto& serviceRegistry = config_.getCoreSystems().serviceRegistry;
    
    if (consumedServices.empty()) {
        log("No consumed services to discover.");
        return true; // Assuming discovering no services is not an error.
    }
    
    discoveredServices_.clear();
    bool ret = true;
    
    for (const auto& service : consumedServices) {
        log("Asking for: " + service + " Service");
        
        json queryJson = createServiceQueryJson(service);
        std::string queryPayload = queryJson.dump();
        
        HttpResponse response = httpClient_.post(
            serviceRegistry.protocol,
            serviceRegistry.host,
            serviceRegistry.port,
            serviceRegistry.path + "/query",
            queryPayload);
        
        if (response.statusCode == 200) {
            log("200 Got Payload: " + response.body);
            
            if (!parseServiceQueryResponse(response.body, service)) {
                log("Failed to parse or no services found");
                ret = false;
            }
        } else {
            log("Error during service discovery: " + std::to_string(response.statusCode));
            log(response.body);
            ret = false;
        }
    }
    
    return ret;
}

std::string ArrowheadManager::consumeService(const std::string& serviceName) {
    for (const auto& service : discoveredServices_) {
        if (service.serviceName == serviceName) {
            // Construct the service URL
            std::string serviceUrl = "http://" + service.providerAddress + ":" + 
                                   std::to_string(service.providerPort) + service.serviceUri;
            
            HttpResponse response = httpClient_.get(
                "http",  // Assuming HTTP protocol
                service.providerAddress, 
                service.providerPort, 
                service.serviceUri);
            
            if (response.statusCode == 200) {
                log("Service response: " + response.body);
                return response.body; // Return the successful payload
            } else {
                log("Failed to consume " + serviceName + " service, HTTP response code: " + 
                    std::to_string(response.statusCode));
                log("Attempting to re-discover...");
                servicesDiscovered_ = false;
                return ""; // Service consumption failed
            }
        }
    }
    
    log("Service not found: " + serviceName);
    return ""; // Service name not matched
}

bool ArrowheadManager::consumeService(
    const std::string& serviceName, 
    const std::string& argumentName, 
    int argumentValue) {
    
    for (const auto& service : discoveredServices_) {
        if (service.serviceName == serviceName) {
            // Construct the query string
            std::string query = argumentName + "=" + std::to_string(argumentValue);
            
            HttpResponse response = httpClient_.get(
                "http",  // Assuming HTTP protocol
                service.providerAddress, 
                service.providerPort, 
                service.serviceUri,
                query);
            
            if (response.statusCode == 200) {
                log("Service response: " + response.body);
                
                try {
                    // Parse the JSON response
                    json doc = json::parse(response.body);
                    
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
                    std::to_string(response.statusCode));
                log(response.body);
                servicesDiscovered_ = false;
                return false; // Service consumption failed
            }
        }
    }
    
    log("Service not found: " + serviceName);
    return false; // Service name not matched
}

void ArrowheadManager::log(const std::string& message) {
    std::cout << message << std::endl;
}

} // namespace arrowhead