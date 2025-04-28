#pragma once

#include <string>
#include <map>
#include <list>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <memory>
#include <functional>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Http_Handler.hpp"
#include "Https_Handler.hpp"

using namespace std;
using json = nlohmann::json;

/**
 * @brief Interface for communicating with the Arrowhead Orchestrator
 *
 * This class provides functionality to communicate with the Orchestrator
 * service of the Arrowhead Framework. It handles the configuration loading,
 * HTTP/HTTPS server initialization, and orchestration request sending.
 */
class OrchestratorInterface : public Http_Handler, public Https_Handler {
private:
    // Configuration values
    json config;
    string OR_BASE_URI;
    string OR_BASE_URI_HTTPS;
    string ADDRESS;
    string ADDRESS6;
    unsigned short PORT;
    string URI;

    bool LoadConfigFile(const string& filename);

protected:
    // Last orchestration response
    json lastOrchestrationResponse;
    bool lastOrchestrationSuccessful = false;

public:
    // Consumer identifier for orchestration requests
    string sConsumerID;
    
    // System information
    string systemName;
    string systemAddress;
    int systemPort;
    bool isSecure = false;
    string authenticationInfo;

    OrchestratorInterface();
    OrchestratorInterface(string config_file);
    virtual ~OrchestratorInterface();

    /**
     * @brief Initialize the OrchestratorInterface with configuration from JSON file
     * 
     * @param config_file Path to the JSON configuration file
     * @return true if initialization was successful, false otherwise
     */
    bool init_OrchestratorInterface(string config_file);
    
    /**
     * @brief Get a configuration value as string
     * 
     * @param section The section of the configuration 
     * @param key The key of the configuration value
     * @param defaultValue The default value to return if the key is not found
     * @return string The configuration value or default value
     */
    string getConfigValue(const string& section, const string& key, const string& defaultValue = "");
    
    /**
     * @brief Get a configuration value as integer
     * 
     * @param section The section of the configuration
     * @param key The key of the configuration value
     * @param defaultValue The default value to return if the key is not found
     * @return int The configuration value as integer or default value
     */
    int getConfigValueInt(const string& section, const string& key, int defaultValue = 0);
    
    /**
     * @brief Initialize the system information for orchestration requests
     * 
     * @param systemName Name of the consumer system
     * @param systemAddress Address of the consumer system
     * @param systemPort Port of the consumer system
     * @param isSecure Whether the system uses secure communication
     * @param authenticationInfo Authentication information for secure communication
     * @return true if successful
     */
    bool init_SystemInfo(const string& systemName, const string& systemAddress, 
                        int systemPort, bool isSecure = false, 
                        const string& authenticationInfo = "");
    
    /**
     * @brief Deinitialize and clean up resources
     * 
     * @return int Error code (0 for success)
     */
    int deinit();

    /**
     * @brief Send an orchestration request to the Arrowhead Orchestrator
     * 
     * @param requestForm JSON request body for the orchestration
     * @param _bSecureArrowheadInterface Whether to use HTTPS (true) or HTTP (false)
     * @return int HTTP response code (200 for success)
     */
    int sendOrchestrationRequest(string requestForm, bool _bSecureArrowheadInterface);

    /**
     * @brief Create an orchestration request for a specific service
     * 
     * @param serviceDefinition Service definition to request
     * @param interfaces Supported interfaces
     * @param metadata Required metadata
     * @return string JSON request string
     */
    string createOrchestrationRequest(const string& serviceDefinition,
                                     const vector<string>& interfaces,
                                     const map<string, string>& metadata = {});

    /**
     * @brief Get the last orchestration response
     * 
     * @return json The last orchestration response as JSON
     */
    json getLastOrchestrationResponse() const;

    /**
     * @brief Check if the last orchestration was successful
     * 
     * @return true if successful
     */
    bool wasOrchestrationSuccessful() const;

    /**
     * @brief Get the HTTP address of this system
     * 
     * @return string HTTP address with port
     */
    string getSystemAddress() const;

    /**
     * @brief Callback for HTTP responses
     * 
     * @param ptr Pointer to response data
     * @param size Size of response data
     * @return size_t Size of processed data
     */
    size_t httpResponseCallback(char *ptr, size_t size) override;
    
    /**
     * @brief Callback for HTTPS responses
     * 
     * @param ptr Pointer to response data
     * @param size Size of response data
     * @return size_t Size of processed data
     */
    size_t httpsResponseCallback(char *ptr, size_t size) override;
    
    /**
     * @brief Process orchestration response
     * 
     * This callback is called when an orchestration response is received.
     * Derived classes should override this method to process the response.
     * 
     * @param ptr Pointer to response data
     * @param size Size of response data
     * @return size_t Size of processed data
     */
    virtual size_t Callback_OrchestrationResponse(char *ptr, size_t size);
};