#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "../Interface/OrchestratorInterface.hpp"

using namespace std;

/**
 * @brief Represents an Arrowhead service instance that has been discovered
 */
struct ServiceInstance {
    string serviceDefinition;      // Service definition name
    string providerName;           // Provider system name
    string providerAddress;        // Provider IP address
    int providerPort;              // Provider port
    string serviceUri;             // The URI path for the service 
    string interfaceType;          // Interface type (e.g., "HTTP-SECURE-JSON")
    map<string, string> metadata;  // Additional metadata
    bool isSecure;                 // Whether this is a secure service
    string authenticationInfo;     // Authentication info if secure service
};

/**
 * @brief Service consumer class for the Arrowhead Framework
 * 
 * This class handles service orchestration, discovery, and consumption.
 * It communicates with the Orchestrator to find services that match
 * specified requirements, and provides methods to call those services.
 */
class ServiceConsumer : public OrchestratorInterface {
private:
    // Discovered services (after orchestration)
    vector<ServiceInstance> discoveredServices;
    
    // Override callback for orchestration responses
    size_t Callback_OrchestrationResponse(char *ptr, size_t size) override;
    
    // Parse orchestration response
    bool parseOrchestrationResponse(const string& response);
    
public:
    ServiceConsumer();
    ServiceConsumer(const string& ini_file);
    ~ServiceConsumer();
    
    // Initialize the consumer with system information
    bool initConsumer(const string& systemName, const string& address, int port, 
                     bool isSecure = false, const string& authenticationInfo = "");
    
    // Discover a service using orchestration
    bool discoverService(const string& serviceDefinition, 
                        const vector<string>& interfaces = {"HTTP-INSECURE-JSON"},
                        const map<string, string>& metadata = {});
    
    // Get discovered services matching criteria
    vector<ServiceInstance> getDiscoveredServices(const string& serviceDefinition = "");
    
    // Consume a service via HTTP GET
    bool consumeService(const string& serviceDefinition, string& response);
    
    // Consume a service via HTTP POST
    bool consumeService(const string& serviceDefinition, const string& payload, string& response);
    
    // Consume a service with query parameters (HTTP GET)
    bool consumeServiceWithParams(const string& serviceDefinition, 
                                 const map<string, string>& queryParams, 
                                 string& response);
                                 
    // Low-level service consumption methods
    bool consumeServiceByInstance(const ServiceInstance& service, string& response);
    bool consumeServiceByInstance(const ServiceInstance& service, const string& payload, string& response);
    bool consumeServiceByInstanceWithParams(const ServiceInstance& service, 
                                          const map<string, string>& queryParams, 
                                          string& response);
};