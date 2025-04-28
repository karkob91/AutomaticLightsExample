#include "OrchestratorInterface.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

OrchestratorInterface::OrchestratorInterface() {
    // Default constructor
}

OrchestratorInterface::OrchestratorInterface(string config_file) {
    init_OrchestratorInterface(config_file);
}

OrchestratorInterface::~OrchestratorInterface() {
    deinit();
}

// Overload Http_Handler and Https_Handler callback functionality
size_t OrchestratorInterface::httpResponseCallback(char *ptr, size_t size) {
    return Callback_OrchestrationResponse(ptr, size);
}

size_t OrchestratorInterface::httpsResponseCallback(char *ptr, size_t size) {
    return Callback_OrchestrationResponse(ptr, size);
}

size_t OrchestratorInterface::Callback_OrchestrationResponse(char *ptr, size_t size) {
    // Default implementation - this should be overridden in derived classes
    printf("Callback_OrchestrationResponse -- need to overwrite\n");
    
    try {
        // Try to parse the response as JSON
        string responseStr(ptr, size);
        lastOrchestrationResponse = json::parse(responseStr);
        
        // Check if the response contains services
        if (lastOrchestrationResponse.contains("response") && 
            lastOrchestrationResponse["response"].is_array() &&
            !lastOrchestrationResponse["response"].empty()) {
            lastOrchestrationSuccessful = true;
        } else {
            lastOrchestrationSuccessful = false;
        }
    } catch (...) {
        // If parsing fails, set orchestration as unsuccessful
        lastOrchestrationSuccessful = false;
    }
    
    return size;
}

bool OrchestratorInterface::LoadConfigFile(const string& filename) {
    try {
        // Open the file
        std::ifstream configFile(filename);
        if (!configFile.is_open()) {
            std::cerr << "Error: Could not open configuration file: " << filename << std::endl;
            return false;
        }
        
        // Parse JSON from file
        configFile >> config;
        
        // Output parsed config for debugging
        std::cout << "Loaded configuration from " << filename << ":" << std::endl;
        std::cout << config.dump(2) << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON configuration: " << e.what() << std::endl;
        return false;
    }
}

string OrchestratorInterface::getConfigValue(const string& section, const string& key, const string& defaultValue) {
    try {
        if (config.contains(section) && config[section].contains(key)) {
            return config[section][key].get<string>();
        }
    } catch (...) {
        // If any error occurs, return the default value
    }
    return defaultValue;
}

int OrchestratorInterface::getConfigValueInt(const string& section, const string& key, int defaultValue) {
    try {
        if (config.contains(section) && config[section].contains(key)) {
            return config[section][key].get<int>();
        }
    } catch (...) {
        // If any error occurs, return the default value
    }
    return defaultValue;
}

bool OrchestratorInterface::init_OrchestratorInterface(string config_file) {
    if (!LoadConfigFile(config_file)) {
        printf("Error: Cannot load configuration file: %s\n", config_file.c_str());
        return false;
    }

    OR_BASE_URI = getConfigValue("Server", "or_base_uri", 
                               "http://arrowhead.tmit.bme.hu:8440/orchestrator/orchestration");
    OR_BASE_URI_HTTPS = getConfigValue("Server", "or_base_uri_https", 
                                     "https://arrowhead.tmit.bme.hu:8441/orchestrator/orchestration");
    ADDRESS = getConfigValue("Server", "address", "10.0.0.11");
    ADDRESS6 = getConfigValue("Server", "address6", "[::1]");
    PORT = getConfigValueInt("Server", "port", 8453);

    // Set system info from config if available
    string configSystemName = getConfigValue("System", "system_name", "");
    if (!configSystemName.empty()) {
        systemName = configSystemName;
    }

    // Check for secure configuration
    string secureStr = getConfigValue("Security", "use_secure", "false");
    isSecure = (secureStr == "true" || secureStr == "1");

    if (ADDRESS.size() != 0) {
        URI = "http://" + ADDRESS + ":" + to_string(PORT);

        if (MakeServer(PORT)) {
            printf("Error: Unable to start HTTP Server (%s:%d)!\n", ADDRESS.c_str(), PORT);
            return false;
        }

        printf("\nOrchestratorInterface started - %s:%d\n", ADDRESS.c_str(), PORT);
        systemAddress = ADDRESS;
    } else {
        printf("Warning: Could not parse IPv4 address from config, trying to use IPv6!\n");
        URI = "http://" + ADDRESS6 + ":" + to_string(PORT);

        if (MakeServer(PORT)) {
            printf("Error: Unable to start HTTP Server (%s:%d)!\n", ADDRESS6.c_str(), PORT);
            return false;
        }

        printf("\nOrchestratorInterface started - %s:%d\n", ADDRESS6.c_str(), PORT);
        systemAddress = ADDRESS6;
    }
    
    systemPort = PORT;
    return true;
}

bool OrchestratorInterface::init_SystemInfo(const string& sysName, const string& sysAddress, 
                                          int sysPort, bool secure, 
                                          const string& authInfo) {
    systemName = sysName;
    systemAddress = sysAddress;
    systemPort = sysPort;
    isSecure = secure;
    authenticationInfo = authInfo;
    
    return true;
}

int OrchestratorInterface::deinit() {
    KillServer();
    return 0;
}

int OrchestratorInterface::sendOrchestrationRequest(string requestForm, bool _bSecureArrowheadInterface) {
    if (_bSecureArrowheadInterface)
        return SendHttpsRequest(requestForm, OR_BASE_URI_HTTPS, "POST");
    else
        return SendRequest(requestForm, OR_BASE_URI, "POST");
}

string OrchestratorInterface::createOrchestrationRequest(const string& serviceDefinition, 
                                                      const vector<string>& interfaces,
                                                      const map<string, string>& metadata) {
    // Create orchestration request JSON according to Arrowhead specification
    json orchestrationRequest;
    
    // Set requester system information
    json requesterSystem;
    requesterSystem["systemName"] = this->systemName;
    requesterSystem["address"] = this->systemAddress;
    requesterSystem["port"] = this->systemPort;
    
    if (this->isSecure && !this->authenticationInfo.empty()) {
        requesterSystem["authenticationInfo"] = this->authenticationInfo;
    }
    
    orchestrationRequest["requesterSystem"] = requesterSystem;
    
    // Set requested service
    json requestedService;
    requestedService["serviceDefinitionRequirement"] = serviceDefinition;
    
    // Add interface requirements if specified
    if (!interfaces.empty()) {
        json interfaceRequirements = json::array();
        for (const auto& interface : interfaces) {
            interfaceRequirements.push_back(interface);
        }
        requestedService["interfaceRequirements"] = interfaceRequirements;
    }
    
    // Add metadata requirements if specified
    if (!metadata.empty()) {
        json metadataRequirements = json::object();
        for (const auto& [key, value] : metadata) {
            metadataRequirements[key] = value;
        }
        requestedService["metadataRequirements"] = metadataRequirements;
    }
    
    // Set orchestration flags
    json orchestrationFlags;
    orchestrationFlags["overrideStore"] = true;
    orchestrationFlags["matchmaking"] = true;
    orchestrationFlags["metadataSearch"] = !metadata.empty();
    orchestrationFlags["triggerInterCloud"] = false;
    orchestrationFlags["pingProviders"] = true;
    
    orchestrationRequest["requestedService"] = requestedService;
    orchestrationRequest["orchestrationFlags"] = orchestrationFlags;
    
    // Convert to string and return
    return orchestrationRequest.dump();
}

json OrchestratorInterface::getLastOrchestrationResponse() const {
    return lastOrchestrationResponse;
}

bool OrchestratorInterface::wasOrchestrationSuccessful() const {
    return lastOrchestrationSuccessful;
}

string OrchestratorInterface::getSystemAddress() const {
    if (ADDRESS.size() != 0) {
        return "http://" + ADDRESS + ":" + to_string(PORT);
    } else {
        return "http://" + ADDRESS6 + ":" + to_string(PORT);
    }
}