#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include "ServiceConsumer.h"

/**
 * @brief High-level manager for service orchestration and consumption
 * 
 * This class provides a simplified interface for working with the Arrowhead Framework,
 * managing both service providers and consumers in a unified API. It's designed to be
 * easily integrated with Verum Dezyne models.
 */
class OrchestrationManager {
private:
    // Consumer instance
    std::unique_ptr<ServiceConsumer> consumer;
    
    // Cache of discovered services
    std::map<std::string, std::vector<ServiceInstance>> serviceCache;
    std::map<std::string, std::chrono::system_clock::time_point> cacheTimestamps;
    
    // Cache configuration
    bool enableCache;
    std::chrono::seconds cacheTimeout;
    
    // Mutex for thread safety
    std::mutex cacheMutex;
    
    // System information
    std::string systemName;
    std::string systemAddress;
    int systemPort;
    bool isSecure;
    
    // Load configuration from file
    bool loadConfig(const std::string& configFile);
    
    // Check if cached service is still valid
    bool isCacheValid(const std::string& serviceDefinition);
    
public:
    OrchestrationManager();
    OrchestrationManager(const std::string& configFile);
    ~OrchestrationManager();
    
    /**
     * @brief Initialize the orchestration manager
     * 
     * @param systemName Name of this system
     * @param systemAddress IP address of this system
     * @param systemPort Port of this system
     * @param configFile Path to configuration file
     * @param isSecure Whether this system uses secure communication
     * @return bool True if initialization was successful
     */
    bool initialize(const std::string& systemName, 
                   const std::string& systemAddress, 
                   int systemPort,
                   const std::string& configFile = "",
                   bool isSecure = false);
    
    /**
     * @brief Request a service with specific requirements
     * 
     * @param serviceDefinition The service definition to request
     * @param interfaces Acceptable interfaces (default: HTTP-INSECURE-JSON)
     * @param metadata Additional metadata requirements
     * @param forceRefresh Ignore cache and force new orchestration
     * @return vector<ServiceInstance> List of matching service instances
     */
    std::vector<ServiceInstance> requestService(
        const std::string& serviceDefinition,
        const std::vector<std::string>& interfaces = {"HTTP-INSECURE-JSON"},
        const std::map<std::string, std::string>& metadata = {},
        bool forceRefresh = false);
    
    /**
     * @brief Call a service with GET method
     * 
     * @param serviceDefinition Service to call
     * @param response Output parameter for response
     * @return bool True if call was successful
     */
    bool callService(const std::string& serviceDefinition, std::string& response);
    
    /**
     * @brief Call a service with POST method and payload
     * 
     * @param serviceDefinition Service to call
     * @param payload Data to send in the request body
     * @param response Output parameter for response
     * @return bool True if call was successful
     */
    bool callService(const std::string& serviceDefinition, 
                    const std::string& payload, 
                    std::string& response);
    
    /**
     * @brief Call a service with query parameters
     * 
     * @param serviceDefinition Service to call
     * @param queryParams Map of query parameters (key-value pairs)
     * @param response Output parameter for response
     * @return bool True if call was successful
     */
    bool callServiceWithParams(const std::string& serviceDefinition,
                              const std::map<std::string, std::string>& queryParams,
                              std::string& response);
    
    /**
     * @brief Call a specific service instance with GET method
     * 
     * @param serviceInstance Specific service instance to call
     * @param response Output parameter for response
     * @return bool True if call was successful
     */
    bool callServiceInstance(const ServiceInstance& serviceInstance, 
                             std::string& response);
    
    /**
     * @brief Call a specific service instance with POST method
     * 
     * @param serviceInstance Specific service instance to call
     * @param payload Data to send in request body
     * @param response Output parameter for response
     * @return bool True if call was successful
     */
    bool callServiceInstance(const ServiceInstance& serviceInstance,
                             const std::string& payload,
                             std::string& response);
    
    /**
     * @brief Clear service cache
     * 
     * @param serviceDefinition Optional service definition to clear 
     *                          (if empty, clear all cached services)
     */
    void clearServiceCache(const std::string& serviceDefinition = "");
    
    /**
     * @brief Set cache timeout
     * 
     * @param seconds Timeout in seconds
     */
    void setCacheTimeout(int seconds);
    
    /**
     * @brief Enable or disable caching
     * 
     * @param enable True to enable caching, false to disable
     */
    void enableCaching(bool enable);
};