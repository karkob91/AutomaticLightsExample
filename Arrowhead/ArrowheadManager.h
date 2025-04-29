#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include "ArrowheadConfig.h"
#include "HttpClient.h"
#include "ArrowheadServer.h"

namespace arrowhead {

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
     * @brief Construct a new Arrowhead Manager from a configuration
     * @param config The configuration object
     */
    explicit ArrowheadManager(const ArrowheadConfig& config);
    
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
     * @param host The address of the Arrowhead service registry
     * @param port The port of the Arrowhead service registry
     */
    void setServiceRegistry(const std::string& host, int port);
    
    /**
     * @brief Set the Arrowhead system registry address
     * 
     * @param host The address of the Arrowhead system registry 
     * @param port The port of the Arrowhead system registry
     */
    void setSystemRegistry(const std::string& host, int port);
    
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
    bool isSystemRegistered() const { return systemRegistered_; }
    
    /**
     * @brief Check if services are registered with Arrowhead
     * 
     * @return true if registered
     */
    bool areServicesRegistered() const { return servicesRegistered_; }
    
    /**
     * @brief Check if required services were discovered
     * 
     * @return true if discovered
     */
    bool areServicesDiscovered() const { return servicesDiscovered_; }

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
    // Configuration
    ArrowheadConfig config_;
    
    // HTTP client for Arrowhead API calls
    HttpClient httpClient_;
    
    // HTTP server for handling service endpoints
    std::unique_ptr<ArrowheadServer> server_;
    
    // State tracking
    bool systemRegistered_ = false;
    bool servicesRegistered_ = false;
    bool servicesDiscovered_ = false;
    
    // Discovered services
    std::vector<ServiceInfo> discoveredServices_;
    
    // Helper methods
    bool ping();
    bool registerSystem();
    bool registerServices();
    bool deregisterService(const std::string& address, 
                          const std::string& port, 
                          const std::string& serviceDefinition, 
                          const std::string& serviceUri, 
                          const std::string& systemName);
    bool deregisterMatchingServices();
    bool queryAndDeregisterAllServices(const std::string& serviceDefinition, 
                                      const std::string& serviceUri);
    bool queryAndDeregisterAllSystems();
    bool deregisterSystem(const std::string& systemName, 
                         const std::string& address, 
                         int port);
    bool discoverServices();
    
    // JSON helper methods
    using json = nlohmann::json;
    json createSystemRegistrationJson();
    json createServiceRegistrationJson(const std::string& serviceDefinition, 
                                      const std::string& serviceUri);
    json createServiceQueryJson(const std::string& serviceDefinition);
    bool parseServiceQueryResponse(const std::string& response, 
                                 const std::string& serviceDefinition);
    
    // Logging helper
    void log(const std::string& message);
};

} // namespace arrowhead