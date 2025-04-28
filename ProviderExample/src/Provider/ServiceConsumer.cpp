#include "ServiceConsumer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

ServiceConsumer::ServiceConsumer() : OrchestratorInterface()
{
    // Basic initialization
}

ServiceConsumer::ServiceConsumer(const string& config_file) : OrchestratorInterface(config_file)
{
    // Initialization with config file
}

ServiceConsumer::~ServiceConsumer()
{
    // No special cleanup needed
}

bool ServiceConsumer::initConsumer(const string& systemName, const string& address, int port, 
                                 bool isSecure, const string& authenticationInfo)
{
    // Use the init_SystemInfo method from the base class
    return init_SystemInfo(systemName, address, port, isSecure, authenticationInfo);
}

size_t ServiceConsumer::Callback_OrchestrationResponse(char *ptr, size_t size)
{
    // Call base class implementation first
    OrchestratorInterface::Callback_OrchestrationResponse(ptr, size);
    
    // Process the orchestration response
    string responseStr(ptr, size);
    
    // Parse the response
    if (!parseOrchestrationResponse(responseStr)) {
        printf("Failed to parse orchestration response\n");
    }
    
    return size;
}

bool ServiceConsumer::parseOrchestrationResponse(const string& response)
{
    try {
        // Parse the JSON response
        json responseJson = json::parse(response);
        
        // Clear previous discovered services
        discoveredServices.clear();
        
        // Process the response according to the Arrowhead orchestration format
        if (!responseJson.contains("response") || !responseJson["response"].is_array()) {
            printf("Invalid orchestration response format\n");
            return false;
        }
        
        json responseArray = responseJson["response"];
        
        if (responseArray.empty()) {
            printf("No services found in orchestration response\n");
            return false;
        }
        
        // Process each service in the response
        for (const auto& serviceJson : responseArray) {
            ServiceInstance service;
            
            // Extract basic service information
            if (serviceJson.contains("service") && serviceJson["service"].contains("serviceDefinition")) {
                service.serviceDefinition = serviceJson["service"]["serviceDefinition"].get<string>();
            } else {
                printf("Warning: Service without serviceDefinition in response\n");
                continue;
            }
            
            // Extract provider information
            if (serviceJson.contains("provider")) {
                auto& provider = serviceJson["provider"];
                if (provider.contains("systemName")) {
                    service.providerName = provider["systemName"].get<string>();
                }
                if (provider.contains("address")) {
                    service.providerAddress = provider["address"].get<string>();
                }
                if (provider.contains("port")) {
                    service.providerPort = provider["port"].get<int>();
                }
                if (provider.contains("authenticationInfo")) {
                    service.authenticationInfo = provider["authenticationInfo"].get<string>();
                }
            }
            
            // Extract service URI
            if (serviceJson.contains("serviceUri")) {
                service.serviceUri = serviceJson["serviceUri"].get<string>();
            }
            
            // Extract service security information
            if (serviceJson.contains("secure")) {
                string secureValue = serviceJson["secure"].get<string>();
                service.isSecure = (secureValue == "TOKEN" || secureValue == "CERTIFICATE");
            } else {
                service.isSecure = false;
            }
            
            // Extract interface information
            if (serviceJson.contains("interfaces") && serviceJson["interfaces"].is_array() && 
                !serviceJson["interfaces"].empty()) {
                service.interfaceType = serviceJson["interfaces"][0].get<string>();
            }
            
            // Extract metadata if available
            if (serviceJson.contains("metadata") && serviceJson["metadata"].is_object()) {
                for (auto& [key, value] : serviceJson["metadata"].items()) {
                    service.metadata[key] = value.get<string>();
                }
            }
            
            // Add service to the discovered list
            discoveredServices.push_back(service);
        }
        
        printf("Successfully discovered %zu services\n", discoveredServices.size());
        return true;
    } catch (const exception& e) {
        printf("Exception parsing orchestration response: %s\n", e.what());
        return false;
    }
}

bool ServiceConsumer::discoverService(const string& serviceDefinition, 
                                    const vector<string>& interfaces,
                                    const map<string, string>& metadata)
{
    // Create the orchestration request using the base class method
    string requestForm = createOrchestrationRequest(serviceDefinition, interfaces, metadata);
    
    // Send orchestration request
    int result = sendOrchestrationRequest(requestForm, isSecure);
    
    // Check if the request was successful
    if (result != 200) {
        printf("Orchestration request failed with code: %d\n", result);
        return false;
    }
    
    // Check if any services were discovered
    return !discoveredServices.empty();
}

vector<ServiceInstance> ServiceConsumer::getDiscoveredServices(const string& serviceDefinition)
{
    if (serviceDefinition.empty()) {
        return discoveredServices;
    }
    
    // Filter services by service definition
    vector<ServiceInstance> filteredServices;
    
    for (const auto& service : discoveredServices) {
        if (service.serviceDefinition == serviceDefinition) {
            filteredServices.push_back(service);
        }
    }
    
    return filteredServices;
}

bool ServiceConsumer::consumeService(const string& serviceDefinition, string& response)
{
    // Find a service matching the service definition
    auto services = getDiscoveredServices(serviceDefinition);
    
    if (services.empty()) {
        printf("No services found for definition: %s\n", serviceDefinition.c_str());
        return false;
    }
    
    // Use the first matching service
    return consumeServiceByInstance(services[0], response);
}

bool ServiceConsumer::consumeService(const string& serviceDefinition, const string& payload, string& response)
{
    // Find a service matching the service definition
    auto services = getDiscoveredServices(serviceDefinition);
    
    if (services.empty()) {
        printf("No services found for definition: %s\n", serviceDefinition.c_str());
        return false;
    }
    
    // Use the first matching service
    return consumeServiceByInstance(services[0], payload, response);
}

bool ServiceConsumer::consumeServiceWithParams(const string& serviceDefinition, 
                                             const map<string, string>& queryParams, 
                                             string& response)
{
    // Find a service matching the service definition
    auto services = getDiscoveredServices(serviceDefinition);
    
    if (services.empty()) {
        printf("No services found for definition: %s\n", serviceDefinition.c_str());
        return false;
    }
    
    // Use the first matching service
    return consumeServiceByInstanceWithParams(services[0], queryParams, response);
}

bool ServiceConsumer::consumeServiceByInstance(const ServiceInstance& service, string& response)
{
    try {
        // Create Boost.Asio IO context
        net::io_context ioc;
        
        // Create a resolver to lookup the host
        tcp::resolver resolver(ioc);
        
        // Create a stream to the service provider
        beast::tcp_stream stream(ioc);
        
        // Build the URL for the service
        string host = service.providerAddress;
        string port = std::to_string(service.providerPort);
        string target = service.serviceUri;
        
        // If target doesn't start with /, add it
        if (!target.empty() && target[0] != '/') {
            target = "/" + target;
        }
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Connect to the service provider
        stream.connect(results);
        
        // Set up an HTTP GET request
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
        // If secure service, add authentication
        if (service.isSecure && !service.authenticationInfo.empty()) {
            req.set(http::field::authorization, "Bearer " + service.authenticationInfo);
        }
        
        // Send the HTTP request
        http::write(stream, req);
        
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;
        
        // Declare a container to hold the response
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);
        
        // Write the response to the output parameter
        response = res.body();
        
        // Close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        // If we get here, it means the response was received successfully
        return true;
    }
    catch(const std::exception& e) {
        printf("Error consuming service: %s\n", e.what());
        return false;
    }
}

bool ServiceConsumer::consumeServiceByInstance(const ServiceInstance& service, const string& payload, string& response)
{
    try {
        // Create Boost.Asio IO context
        net::io_context ioc;
        
        // Create a resolver to lookup the host
        tcp::resolver resolver(ioc);
        
        // Create a stream to the service provider
        beast::tcp_stream stream(ioc);
        
        // Build the URL for the service
        string host = service.providerAddress;
        string port = std::to_string(service.providerPort);
        string target = service.serviceUri;
        
        // If target doesn't start with /, add it
        if (!target.empty() && target[0] != '/') {
            target = "/" + target;
        }
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Connect to the service provider
        stream.connect(results);
        
        // Set up an HTTP POST request
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        
        // Set the payload
        req.body() = payload;
        req.prepare_payload();
        
        // If secure service, add authentication
        if (service.isSecure && !service.authenticationInfo.empty()) {
            req.set(http::field::authorization, "Bearer " + service.authenticationInfo);
        }
        
        // Send the HTTP request
        http::write(stream, req);
        
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;
        
        // Declare a container to hold the response
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);
        
        // Write the response to the output parameter
        response = res.body();
        
        // Close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        // If we get here, it means the response was received successfully
        return true;
    }
    catch(const std::exception& e) {
        printf("Error consuming service: %s\n", e.what());
        return false;
    }
}

bool ServiceConsumer::consumeServiceByInstanceWithParams(const ServiceInstance& service, 
                                                      const map<string, string>& queryParams, 
                                                      string& response)
{
    try {
        // Create Boost.Asio IO context
        net::io_context ioc;
        
        // Create a resolver to lookup the host
        tcp::resolver resolver(ioc);
        
        // Create a stream to the service provider
        beast::tcp_stream stream(ioc);
        
        // Build the URL for the service
        string host = service.providerAddress;
        string port = std::to_string(service.providerPort);
        string target = service.serviceUri;
        
        // If target doesn't start with /, add it
        if (!target.empty() && target[0] != '/') {
            target = "/" + target;
        }
        
        // Add query parameters
        if (!queryParams.empty()) {
            target += "?";
            bool first = true;
            
            for (const auto& [key, value] : queryParams) {
                if (!first) {
                    target += "&";
                }
                
                // Simple URL encoding for key and value
                string encodedKey = key;
                string encodedValue = value;
                
                // Replace spaces with %20, etc.
                // This is a simplified encoding - in production you'd want a proper URL encoder
                auto encodeComponent = [](const string& s) -> string {
                    string result;
                    for (char c : s) {
                        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                            result += c;
                        } else if (c == ' ') {
                            result += "%20";
                        } else {
                            char hex[4];
                            sprintf(hex, "%%%02X", (unsigned char)c);
                            result += hex;
                        }
                    }
                    return result;
                };
                
                target += encodeComponent(key) + "=" + encodeComponent(value);
                first = false;
            }
        }
        
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        
        // Connect to the service provider
        stream.connect(results);
        
        // Set up an HTTP GET request
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
        // If secure service, add authentication
        if (service.isSecure && !service.authenticationInfo.empty()) {
            req.set(http::field::authorization, "Bearer " + service.authenticationInfo);
        }
        
        // Send the HTTP request
        http::write(stream, req);
        
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;
        
        // Declare a container to hold the response
        http::response<http::string_body> res;
        
        // Receive the HTTP response
        http::read(stream, buffer, res);
        
        // Write the response to the output parameter
        response = res.body();
        
        // Close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        
        // If we get here, it means the response was received successfully
        return true;
    }
    catch(const std::exception& e) {
        printf("Error consuming service with parameters: %s\n", e.what());
        return false;
    }
}