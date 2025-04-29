#pragma once

#include <string>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

namespace arrowhead {

/**
 * @brief Configuration for an endpoint in the Arrowhead service registry
 */
struct EndpointConfig {
    std::string protocol; // e.g., "http" or "https"
    std::string host;     // hostname or IP address
    int port;
    std::string path;     // e.g., "/serviceregistry"
    
    // Get a full URL from this endpoint
    std::string getUrl() const {
        return protocol + "://" + host + ":" + std::to_string(port) + path;
    }
};

/**
 * @brief Configuration for an Arrowhead system
 */
struct SystemConfig {
    std::string name;
    std::string address;
    int port;
    std::string macAddress;
};

/**
 * @brief Configuration for an Arrowhead service
 */
struct ServiceConfig {
    std::string definition;
    std::string uri;
};

/**
 * @brief Configuration for Arrowhead core systems
 */
struct CoreSystemConfig {
    EndpointConfig serviceRegistry;
    EndpointConfig systemRegistry;
};

/**
 * @brief Class to load/save Arrowhead configuration from/to JSON
 */
class ArrowheadConfig {
public:
    /**
     * @brief Default constructor with sensible defaults
     */
    ArrowheadConfig();
    
    /**
     * @brief Constructor with initial configuration
     */
    ArrowheadConfig(const SystemConfig& system,
                   const std::vector<ServiceConfig>& providedServices,
                   const std::vector<std::string>& consumedServices,
                   const CoreSystemConfig& coreSystems);
    
    /**
     * @brief Load configuration from a JSON file
     * @param filename The JSON file to load
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to a JSON file
     * @param filename The JSON file to save to
     * @return true if successful, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Load configuration from a JSON string
     * @param jsonString The JSON string to load from
     * @return true if successful, false otherwise
     */
    bool loadFromString(const std::string& jsonString);
    
    /**
     * @brief Save configuration to a JSON string
     * @return The JSON string
     */
    std::string saveToString() const;
    
    // Getters
    const SystemConfig& getSystem() const { return system_; }
    const std::vector<ServiceConfig>& getProvidedServices() const { return providedServices_; }
    const std::vector<std::string>& getConsumedServices() const { return consumedServices_; }
    const CoreSystemConfig& getCoreSystems() const { return coreSystems_; }
    
    // Setters
    void setSystem(const SystemConfig& system) { system_ = system; }
    void setProvidedServices(const std::vector<ServiceConfig>& services) { providedServices_ = services; }
    void setConsumedServices(const std::vector<std::string>& services) { consumedServices_ = services; }
    void setCoreSystems(const CoreSystemConfig& coreSystems) { coreSystems_ = coreSystems; }
    
    /**
     * @brief Add a provided service
     * @param definition The service definition
     * @param uri The service URI
     */
    void addProvidedService(const std::string& definition, const std::string& uri);
    
    /**
     * @brief Add a consumed service
     * @param definition The service definition
     */
    void addConsumedService(const std::string& definition);

private:
    SystemConfig system_;
    std::vector<ServiceConfig> providedServices_;
    std::vector<std::string> consumedServices_;
    CoreSystemConfig coreSystems_;
};

} // namespace arrowhead