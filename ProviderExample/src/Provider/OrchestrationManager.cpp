#include "OrchestrationManager.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

OrchestrationManager::OrchestrationManager() 
    : enableCache(true), 
      cacheTimeout(std::chrono::seconds(300)) // Default 5 minutes
{
    consumer = std::make_unique<ServiceConsumer>();
}

OrchestrationManager::OrchestrationManager(const std::string& configFile) 
    : enableCache(true), 
      cacheTimeout(std::chrono::seconds(300)) // Default 5 minutes
{
    consumer = std::make_unique<ServiceConsumer>(configFile);
    loadConfig(configFile);
}

OrchestrationManager::~OrchestrationManager() {
    // Cleanup resources if needed
}

bool OrchestrationManager::loadConfig(const std::string& configFile) {
    if (configFile.empty()) {
        return false;
    }
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << configFile << std::endl;
        return false;
    }
    
    // Very basic INI parser - for a more robust solution consider using a library
    std::string line;
    std::string section;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Section header
        if (line[0] == '[' && line[line.size() - 1] == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        
        // Key-value pairs
        size_t delimPos = line.find('=');
        if (delimPos != std::string::npos) {
            std::string key = line.substr(0, delimPos);
            std::string value = line.substr(delimPos + 1);
            
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Process Cache section settings
            if (section == "Cache") {
                if (key == "enable_cache") {
                    enableCache = (value == "true" || value == "1");
                } else if (key == "cache_timeout") {
                    try {
                        cacheTimeout = std::chrono::seconds(std::stoi(value));
                    } catch (...) {
                        std::cerr << "Invalid cache_timeout value: " << value << std::endl;
                    }
                }
            }
        }
    }
    
    return true;
}

bool OrchestrationManager::initialize(const std::string& systemName, 
                                    const std::string& systemAddress, 
                                    int systemPort,
                                    const std::string& configFile,
                                    bool isSecure) {
    this->systemName = systemName;
    this->systemAddress = systemAddress;
    this->systemPort = systemPort;
    this->isSecure = isSecure;
    
    // If config file provided, load it
    if (!configFile.empty()) {
        loadConfig(configFile);
    }
    
    // Initialize the service consumer
    return consumer->initConsumer(systemName, systemAddress, systemPort, isSecure);
}

bool OrchestrationManager::isCacheValid(const std::string& serviceDefinition) {
    if (!enableCache) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto it = cacheTimestamps.find(serviceDefinition);
    
    if (it == cacheTimestamps.end()) {
        return false;
    }
    
    auto elapsed = now - it->second;
    return elapsed < cacheTimeout;
}

std::vector<ServiceInstance> OrchestrationManager::requestService(
    const std::string& serviceDefinition,
    const std::vector<std::string>& interfaces,
    const std::map<std::string, std::string>& metadata,
    bool forceRefresh) {
    
    // Check cache first if not forced to refresh
    if (!forceRefresh) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (isCacheValid(serviceDefinition)) {
            auto it = serviceCache.find(serviceDefinition);
            if (it != serviceCache.end()) {
                return it->second;
            }
        }
    }
    
    // Not in cache or forced refresh, perform orchestration
    if (!consumer->discoverService(serviceDefinition, interfaces, metadata)) {
        std::cerr << "Failed to discover service: " << serviceDefinition << std::endl;
        return std::vector<ServiceInstance>();
    }
    
    // Get discovered services
    auto services = consumer->getDiscoveredServices(serviceDefinition);
    
    // Update cache
    if (enableCache && !services.empty()) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        serviceCache[serviceDefinition] = services;
        cacheTimestamps[serviceDefinition] = std::chrono::system_clock::now();
    }
    
    return services;
}

bool OrchestrationManager::callService(const std::string& serviceDefinition, std::string& response) {
    // First ensure we have discovered the service
    std::vector<ServiceInstance> services;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (isCacheValid(serviceDefinition)) {
            auto it = serviceCache.find(serviceDefinition);
            if (it != serviceCache.end()) {
                services = it->second;
            }
        }
    }
    
    // If not in cache, discover it
    if (services.empty()) {
        services = requestService(serviceDefinition);
        if (services.empty()) {
            std::cerr << "No services found for: " << serviceDefinition << std::endl;
            return false;
        }
    }
    
    // Use the first service instance
    return consumer->consumeServiceByInstance(services[0], response);
}

bool OrchestrationManager::callService(const std::string& serviceDefinition, 
                                      const std::string& payload, 
                                      std::string& response) {
    // First ensure we have discovered the service
    std::vector<ServiceInstance> services;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (isCacheValid(serviceDefinition)) {
            auto it = serviceCache.find(serviceDefinition);
            if (it != serviceCache.end()) {
                services = it->second;
            }
        }
    }
    
    // If not in cache, discover it
    if (services.empty()) {
        services = requestService(serviceDefinition);
        if (services.empty()) {
            std::cerr << "No services found for: " << serviceDefinition << std::endl;
            return false;
        }
    }
    
    // Use the first service instance
    return consumer->consumeServiceByInstance(services[0], payload, response);
}

bool OrchestrationManager::callServiceWithParams(const std::string& serviceDefinition,
                                               const std::map<std::string, std::string>& queryParams,
                                               std::string& response) {
    // First ensure we have discovered the service
    std::vector<ServiceInstance> services;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (isCacheValid(serviceDefinition)) {
            auto it = serviceCache.find(serviceDefinition);
            if (it != serviceCache.end()) {
                services = it->second;
            }
        }
    }
    
    // If not in cache, discover it
    if (services.empty()) {
        services = requestService(serviceDefinition);
        if (services.empty()) {
            std::cerr << "No services found for: " << serviceDefinition << std::endl;
            return false;
        }
    }
    
    // Use the first service instance
    return consumer->consumeServiceByInstanceWithParams(services[0], queryParams, response);
}

bool OrchestrationManager::callServiceInstance(const ServiceInstance& serviceInstance, std::string& response) {
    return consumer->consumeServiceByInstance(serviceInstance, response);
}

bool OrchestrationManager::callServiceInstance(const ServiceInstance& serviceInstance,
                                             const std::string& payload,
                                             std::string& response) {
    return consumer->consumeServiceByInstance(serviceInstance, payload, response);
}

void OrchestrationManager::clearServiceCache(const std::string& serviceDefinition) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (serviceDefinition.empty()) {
        // Clear all cache
        serviceCache.clear();
        cacheTimestamps.clear();
    } else {
        // Clear specific service
        serviceCache.erase(serviceDefinition);
        cacheTimestamps.erase(serviceDefinition);
    }
}

void OrchestrationManager::setCacheTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cacheTimeout = std::chrono::seconds(seconds);
}

void OrchestrationManager::enableCaching(bool enable) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    enableCache = enable;
}