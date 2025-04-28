#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include "ArrowheadServer.h"

/**
 * @brief Structure to hold discovered service information
 */
struct ServiceInfo {
    std::string serviceName;
    std::string providerAddress;
    int providerPort;
    std::string serviceUri;
};

/**
 * @brief Cross-platform Arrowhead Framework manager 
 * 
 * This class handles system registration, service registration/deregistration, 
 * service discovery, and service consumption within the Arrowhead Framework.
 * It also provides a built-in HTTP server for handling service endpoints.
 */
class ArrowheadManager {
public:
    /**
     * @brief Construct a new Arrowhead Manager
     * 
     * @param systemName The name of this system in the Arrowhead framework
     * @param systemAddress The address/IP of this system
     * @param systemPort The port this system is running on
     * @param macAddress Optional MAC address for system identification
     * @param provided_services Services this system provides (pairs of definition + URI)
     * @param consumed_services Services this system consumes (definitions)
     */
    ArrowheadManager(const std::string& systemName, 
                    const std::string& systemAddress, 
                    int systemPort,
                    const std::string& macAddress,
                    const std::vector<std::pair<std::string, std::string>>& provided_services,
                    const std::vector<std::string>& consumed_services);
    
    /**
     * @brief Set the Arrowhead service registry address
     * 
     * @param address The address of the Arrowhead service registry
     * @param port The port of the Arrowhead service registry
     */
    void setArrowheadServiceRegistry(const std::string& address, int port);
    
    /**
     * @brief Set the Arrowhead system registry address
     * 
     * @param address The address of the Arrowhead system registry 
     * @param port The port of the Arrowhead system registry
     */
    void setArrowheadSystemRegistry(const std::string& address, int port);
    
    /**
     * @brief Register the system and its services with Arrowhead
     * 
     * This function handles system registration, service registration,
     * and service discovery in a single call.
     * 
     * @return true if all operations were successful
     * @return false if any operation failed
     */
    bool registerSystemAndServices();
    
    /**
     * @brief Consume a service
     * 
     * @param serviceName The name/definition of the service to consume
     * @return std::string The response from the service
     */
    std::string consumeService(const std::string& serviceName);
    
    /**
     * @brief Consume a service with parameters
     * 
     * @param serviceName The name/definition of the service to consume
     * @param argumentName The name of the query parameter
     * @param argumentValue The value of the query parameter
     * @return bool The boolean result from service consumption
     */
    bool consumeService(const std::string& serviceName, 
                        const std::string& argumentName, 
                        int argumentValue);
    
    /**
     * @brief Check if this system is registered with Arrowhead
     * 
     * @return true if registered
     */
    bool isSystemRegistered() const { return systemRegistered; }
    
    /**
     * @brief Check if services are registered with Arrowhead
     * 
     * @return true if registered
     */
    bool areServicesRegistered() const { return servicesRegistered; }
    
    /**
     * @brief Check if required services were discovered
     * 
     * @return true if discovered
     */
    bool areServicesDiscovered() const { return servicesDiscovered; }

    /**
     * @brief Register a handler for an HTTP endpoint
     * 
     * This method allows you to register a handler function for a specific HTTP endpoint.
     * 
     * @param endpoint The endpoint URI (e.g., "/api_armed")
     * @param method The HTTP method (e.g., "GET", "POST")
     * @param handler The handler function
     */
    void on(const std::string& endpoint, const std::string& method, RequestHandler handler);

    /**
     * @brief Start the HTTP server
     * 
     * @return true if the server started successfully
     */
    bool startServer();

    /**
     * @brief Stop the HTTP server
     */
    void stopServer();

    /**
     * @brief Check if the server is running
     * 
     * @return true if the server is running
     */
    bool isServerRunning() const;
    
private:
    // State tracking
    bool systemRegistered = false;
    bool arrowheadOnline = false;
    bool servicesRegistered = false;
    bool servicesDiscovered = false;
    
    // System information
    std::string systemName;
    std::string systemAddress;
    std::string macAddress;
    int systemPort;
    
    // Service information
    const std::vector<std::pair<std::string, std::string>> provided_services;
    const std::vector<std::string> consumed_services;
    std::vector<ServiceInfo> discovered_services;
    
    // Arrowhead connection information
    std::string arrowheadAddress = "127.0.0.1";
    int serviceRegistryPort = 8443;
    int systemRegistryPort = 8437;
    std::string serviceRegistryURL;
    std::string systemRegistryURL;
    
    // HTTP server for handling service endpoints
    std::unique_ptr<ArrowheadServer> server;
    
    // Helper methods
    std::string encodeUrl(const std::string& str);
    bool deregisterMatchingServices();
    bool registerSystem();
    bool registerServices();
    bool httpPing();
    bool deregisterSystem(const std::string& systemName, 
                          const std::string& address, 
                          int port);
    bool queryAndDeregisterAllSystems();
    bool queryAndDeregisterAllServices(const std::string& serviceDefinition, 
                                      const std::string& serviceUri);
    bool deregisterService(const std::string& address, 
                           const std::string& port, 
                           const std::string& serviceDefinition, 
                           const std::string& serviceUri, 
                           const std::string& systemName);
    bool discoverServices();
    
    // HTTP utility functions
    int httpGet(const std::string& url, std::string& response);
    int httpPost(const std::string& url, const std::string& payload, std::string& response);
    int httpDelete(const std::string& url, std::string& response);
    
    // JSON helper functions
    using json = nlohmann::json;
    json createSystemRegistrationJson();
    json createServiceRegistrationJson(const std::string& serviceDefinition, 
                                      const std::string& serviceUri);
    json createServiceQueryJson(const std::string& serviceDefinition);
    bool parseServiceQueryResponse(const std::string& response, 
                                 const std::string& serviceDefinition);
};