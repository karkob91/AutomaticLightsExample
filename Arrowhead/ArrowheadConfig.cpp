#include "ArrowheadConfig.h"
#include <fstream>
#include <iostream>

namespace arrowhead {

using json = nlohmann::json;

ArrowheadConfig::ArrowheadConfig() {
    // Default system configuration
    system_.name = "DefaultSystem";
    system_.address = "127.0.0.1";
    system_.port = 8080;
    system_.macAddress = "00:00:00:00:00:00";
    
    // Default core systems configuration
    coreSystems_.serviceRegistry.protocol = "http";
    coreSystems_.serviceRegistry.host = "127.0.0.1";
    coreSystems_.serviceRegistry.port = 8443;
    coreSystems_.serviceRegistry.path = "/serviceregistry";
    
    coreSystems_.systemRegistry.protocol = "http";
    coreSystems_.systemRegistry.host = "127.0.0.1";
    coreSystems_.systemRegistry.port = 8437;
    coreSystems_.systemRegistry.path = "/systemregistry";
}

ArrowheadConfig::ArrowheadConfig(
    const SystemConfig& system,
    const std::vector<ServiceConfig>& providedServices,
    const std::vector<std::string>& consumedServices,
    const CoreSystemConfig& coreSystems)
    : system_(system),
      providedServices_(providedServices),
      consumedServices_(consumedServices),
      coreSystems_(coreSystems) {
}

bool ArrowheadConfig::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }
        
        json configJson;
        file >> configJson;
        file.close();
        
        return loadFromString(configJson.dump());
    } catch (const std::exception& e) {
        std::cerr << "Error loading config from file: " << e.what() << std::endl;
        return false;
    }
}

bool ArrowheadConfig::saveToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filename << std::endl;
            return false;
        }
        
        file << saveToString();
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config to file: " << e.what() << std::endl;
        return false;
    }
}

bool ArrowheadConfig::loadFromString(const std::string& jsonString) {
    try {
        json configJson = json::parse(jsonString);
        
        // Load system configuration
        if (configJson.contains("system")) {
            const auto& systemJson = configJson["system"];
            system_.name = systemJson.value("name", "DefaultSystem");
            system_.address = systemJson.value("address", "127.0.0.1");
            system_.port = systemJson.value("port", 8080);
            system_.macAddress = systemJson.value("macAddress", "00:00:00:00:00:00");
        }
        
        // Load provided services
        providedServices_.clear();
        if (configJson.contains("providedServices") && configJson["providedServices"].is_array()) {
            for (const auto& serviceJson : configJson["providedServices"]) {
                ServiceConfig service;
                service.definition = serviceJson.value("definition", "");
                service.uri = serviceJson.value("uri", "");
                if (!service.definition.empty() && !service.uri.empty()) {
                    providedServices_.push_back(service);
                }
            }
        }
        
        // Load consumed services
        consumedServices_.clear();
        if (configJson.contains("consumedServices") && configJson["consumedServices"].is_array()) {
            for (const auto& serviceDefinition : configJson["consumedServices"]) {
                if (serviceDefinition.is_string() && !serviceDefinition.get<std::string>().empty()) {
                    consumedServices_.push_back(serviceDefinition);
                }
            }
        }
        
        // Load core systems configuration
        if (configJson.contains("coreSystems")) {
            const auto& coreJson = configJson["coreSystems"];
            
            // Service Registry
            if (coreJson.contains("serviceRegistry")) {
                const auto& srJson = coreJson["serviceRegistry"];
                coreSystems_.serviceRegistry.protocol = srJson.value("protocol", "http");
                coreSystems_.serviceRegistry.host = srJson.value("host", "127.0.0.1");
                coreSystems_.serviceRegistry.port = srJson.value("port", 8443);
                coreSystems_.serviceRegistry.path = srJson.value("path", "/serviceregistry");
            }
            
            // System Registry
            if (coreJson.contains("systemRegistry")) {
                const auto& sysJson = coreJson["systemRegistry"];
                coreSystems_.systemRegistry.protocol = sysJson.value("protocol", "http");
                coreSystems_.systemRegistry.host = sysJson.value("host", "127.0.0.1");
                coreSystems_.systemRegistry.port = sysJson.value("port", 8437);
                coreSystems_.systemRegistry.path = sysJson.value("path", "/systemregistry");
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string ArrowheadConfig::saveToString() const {
    json configJson;
    
    // System configuration
    json systemJson;
    systemJson["name"] = system_.name;
    systemJson["address"] = system_.address;
    systemJson["port"] = system_.port;
    systemJson["macAddress"] = system_.macAddress;
    configJson["system"] = systemJson;
    
    // Provided services
    json providedServicesJson = json::array();
    for (const auto& service : providedServices_) {
        json serviceJson;
        serviceJson["definition"] = service.definition;
        serviceJson["uri"] = service.uri;
        providedServicesJson.push_back(serviceJson);
    }
    configJson["providedServices"] = providedServicesJson;
    
    // Consumed services
    json consumedServicesJson = json::array();
    for (const auto& service : consumedServices_) {
        consumedServicesJson.push_back(service);
    }
    configJson["consumedServices"] = consumedServicesJson;
    
    // Core systems configuration
    json coreJson;
    
    // Service Registry
    json srJson;
    srJson["protocol"] = coreSystems_.serviceRegistry.protocol;
    srJson["host"] = coreSystems_.serviceRegistry.host;
    srJson["port"] = coreSystems_.serviceRegistry.port;
    srJson["path"] = coreSystems_.serviceRegistry.path;
    coreJson["serviceRegistry"] = srJson;
    
    // System Registry
    json sysJson;
    sysJson["protocol"] = coreSystems_.systemRegistry.protocol;
    sysJson["host"] = coreSystems_.systemRegistry.host;
    sysJson["port"] = coreSystems_.systemRegistry.port;
    sysJson["path"] = coreSystems_.systemRegistry.path;
    coreJson["systemRegistry"] = sysJson;
    
    configJson["coreSystems"] = coreJson;
    
    return configJson.dump(4); // Pretty-print with 4-space indentation
}

void ArrowheadConfig::addProvidedService(const std::string& definition, const std::string& uri) {
    ServiceConfig service;
    service.definition = definition;
    service.uri = uri;
    providedServices_.push_back(service);
}

void ArrowheadConfig::addConsumedService(const std::string& definition) {
    consumedServices_.push_back(definition);
}

} // namespace arrowhead