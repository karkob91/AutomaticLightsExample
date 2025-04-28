#include "ApplicationServiceInterface.hpp"

ApplicationServiceInterface::ApplicationServiceInterface()
{
}

ApplicationServiceInterface::ApplicationServiceInterface(string config_file)
{
    init_ApplicationServiceInterface(config_file);
}

ApplicationServiceInterface::~ApplicationServiceInterface()
{
    deinit();
}

// HTTP_Handler overload
int ApplicationServiceInterface::httpGETCallback(const char *Id, string *pData_str)
{
    return Callback_Serve_HTTP_GET(Id, pData_str);
}

int ApplicationServiceInterface::Callback_Serve_HTTP_GET(const char *Id, string *pData_str)
{
    *pData_str = "5678";
    return 1;
}

// HTTPs_Handler overload
int ApplicationServiceInterface::httpsGETCallback(const char *Id, string *pData_str, string _sToken, string _sSignature, string _clientDistName)
{
    return Callback_Serve_HTTPs_GET(Id, pData_str, _sToken, _sSignature, _clientDistName);
}

int ApplicationServiceInterface::Callback_Serve_HTTPs_GET(const char *Id, string *pData_str, string _sToken, string _sSignature, string _clientDistName)
{
    *pData_str = "5678";
    return 1;
}

bool ApplicationServiceInterface::LoadConfigFile(const string& filename) {
    try {
        // Open the file
        std::ifstream configFile(filename);
        if (!configFile.is_open()) {
            std::cerr << "Error: Could not open configuration file: " << filename << std::endl;
            return false;
        }
        
        // Parse JSON from file
        // configFile >> config;
        
        // Output parsed config for debugging
        // std::cout << "Loaded configuration from " << filename << ":" << std::endl;
        // std::cout << config.dump(2) << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON configuration: " << e.what() << std::endl;
        return false;
    }
}

string ApplicationServiceInterface::GetConfigValue(const string& section, const string& key, const string& defaultValue) {
    try {
        if (config.contains(section) && config[section].contains(key)) {
            return config[section][key].get<string>();
        }
    } catch (...) {
        // If any error occurs, return the default value
    }
    return defaultValue;
}

int ApplicationServiceInterface::GetConfigValueInt(const string& section, const string& key, int defaultValue) {
    try {
        if (config.contains(section) && config[section].contains(key)) {
            return config[section][key].get<int>();
        }
    } catch (...) {
        // If any error occurs, return the default value
    }
    return defaultValue;
}

bool ApplicationServiceInterface::init_ApplicationServiceInterface(string config_file)
{
    if (!LoadConfigFile(config_file)) {
        printf("Error: Cannot load configuration file: %s\n", config_file.c_str());
        return false;
    }

    SR_BASE_URI = GetConfigValue("Server", "sr_base_uri", "http://10.0.0.77:8443/serviceregistry/");
    SR_BASE_URI_HTTPS = GetConfigValue("Server", "sr_base_uri_https", "https://10.0.0.77:8444/serviceregistry/");
    ADDRESS = GetConfigValue("Server", "address", "10.0.0.77");
    ADDRESS6 = GetConfigValue("Server", "address6", "[::1]");
    PORT = GetConfigValueInt("Server", "port", 8452);

    if(ADDRESS.size() != 0) {
        URI = "http://" + ADDRESS + ":" + to_string(PORT);
        HTTPsURI = "https://" + ADDRESS + ":" + to_string(PORT+1);

        if(MakeServer(PORT)) {
            printf("Error: Unable to start HTTP Server (%s:%d)!\n", ADDRESS.c_str(), PORT);
            return false;
        }

        if(MakeHttpsServer(PORT+1)) {
            printf("Error: Unable to start HTTPs Server (%s:%d)!\n", ADDRESS.c_str(), PORT);
            return false;
        }

        printf("\n(HTTP Server) started - %s:%d\n", ADDRESS.c_str(), PORT);
        printf("(HTTPs Server) started - %s:%d\n", ADDRESS.c_str(), PORT+1);
    }
    else {
        printf("Warning: Could not parse IPv4 address from config, trying to use IPv6!\n");

        URI = "http://" + ADDRESS6 + ":" + to_string(PORT);
        HTTPsURI = "https://" + ADDRESS6 + ":" + to_string(PORT+1);

        if(MakeServer(PORT)) {
            printf("Error: Unable to start HTTP Server (%s:%d)!\n", ADDRESS6.c_str(), PORT);
            return false;
        }

        if(MakeHttpsServer(PORT+1)) {
            printf("Error: Unable to start HTTPs Server (%s:%d)!\n", ADDRESS6.c_str(), PORT+1);
            return false;
        }

        printf("\n(HTTP Server) started - %s:%d\n", ADDRESS6.c_str(), PORT);
        printf("\n(HTTPs Server) started - %s:%d\n", ADDRESS6.c_str(), PORT+1);
    }

    return true;
}

int ApplicationServiceInterface::deinit()
{
    KillServer();
    KillHttpsServer();
    return 0;
}

const char *GetHttpPayload(Arrowhead_Data_ext &stAH_data, string ADDRESS, string ADDRESS6, unsigned short PORT)
{
    //Expected content, example:
    /*
    {
     "serviceDefinition": "IndoorTemperature",
     "serviceUri": "temperature",
     "endOfValidity": "2019-12-05T12:00:00",
     "secure": "TOKEN",
     "version": 1,

     "providerSystem":
     {
       "systemName": "InsecureTemperatureSensor",
       "address": "192.168.0.2",
       "port": 8080,
       "authenticationInfo": "eyJhbGciOiJIUzI1Ni..."
     },

     "metadata": {
       "unit": "celsius"
    },

     "interfaces": [
       "HTTP-SECURE-JSON"
     ]
    }
    */
    static std::string payload;
    json jobj = json::object();
    json providerSystem = json::object();

    // Set service definition
    jobj["serviceDefinition"] = stAH_data.sServiceDefinition;
    
    // Set service URI
    jobj["serviceUri"] = stAH_data.sServiceURI;
    
    // Set version
    jobj["version"] = 1;

    // providerSystem section
    providerSystem["systemName"] = stAH_data.sSystemName;
    providerSystem["address"] = ADDRESS.size() != 0 ? ADDRESS : ADDRESS6;

    if(stAH_data.sAuthenticationInfo.size() != 0){
        jobj["secure"] = "TOKEN";
        providerSystem["authenticationInfo"] = stAH_data.sAuthenticationInfo;
    }
    else{
        jobj["secure"] = "NOT_SECURE";
    }

    providerSystem["port"] = PORT;
    jobj["providerSystem"] = providerSystem;

    // Interfaces, Metadata
    json interfaces = json::array();
    
    if(stAH_data.sAuthenticationInfo.size() != 0)
        interfaces.push_back("HTTP-SECURE-JSON");
    else
        interfaces.push_back("HTTP-INSECURE-JSON");
    
    jobj["interfaces"] = interfaces;

    json serviceMetadata = json::object();
    if (stAH_data.vService_Meta.find("unit") != stAH_data.vService_Meta.end()) {
        serviceMetadata["unit"] = stAH_data.vService_Meta.at("unit");
    }

    if(stAH_data.sAuthenticationInfo.size() != 0 && 
       stAH_data.vService_Meta.find("security") != stAH_data.vService_Meta.end()){
        serviceMetadata["security"] = stAH_data.vService_Meta.at("security");
    }

    jobj["metadata"] = serviceMetadata;

    // Return
    printf("\n%s\n", jobj.dump().c_str());
    
    payload = jobj.dump();
    return payload.c_str();
}

int ApplicationServiceInterface::registerToServiceRegistry(Arrowhead_Data_ext &stAH_data, bool _bSecureArrowheadInterface, bool _bProviderIsSecure)
{
    if(_bSecureArrowheadInterface)
          return SendHttpsRequest(GetHttpPayload(stAH_data, ADDRESS, ADDRESS6, _bProviderIsSecure ? PORT+1 : PORT), SR_BASE_URI_HTTPS + "register", "POST");
     else
          return SendRequest(GetHttpPayload(stAH_data, ADDRESS, ADDRESS6, _bProviderIsSecure ? PORT+1 : PORT), SR_BASE_URI + "register", "POST");
}

int ApplicationServiceInterface::unregisterFromServiceRegistry(Arrowhead_Data_ext &stAH_data, bool _bSecureArrowheadInterface, bool _bProviderIsSecure)
{
    // URL encode the service URI if it contains special characters
    std::string encodedServiceUri = stAH_data.sServiceURI;
    
    // Simple URL encoding for the service URI
    auto urlEncode = [](const std::string &s) {
        std::string result;
        for (char c : s) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result += c;
            } else if (c == ' ') {
                result += "%20";
            } else if (c == '/') {
                result += "%2F";  // Encode forward slash
            } else {
                result += '%';
                char hex[3];
                sprintf(hex, "%02X", (unsigned char)c);
                result += hex;
            }
        }
        return result;
    };
    
    encodedServiceUri = urlEncode(encodedServiceUri);
    
    // Build query parameters with ALL required fields
    std::string sParams = "service_definition=" + stAH_data.sServiceDefinition +
                          "&system_name=" + stAH_data.sSystemName +
                          "&address=";

    if(ADDRESS.size())
        sParams += ADDRESS;
    else
        sParams += ADDRESS6;

    sParams += "&port=";

    if(_bProviderIsSecure)
        sParams += std::to_string(PORT+1);
    else
        sParams += std::to_string(PORT);
    
    // Add the service_uri parameter which is required for unregistration
    sParams += "&service_uri=" + encodedServiceUri;

    // Log the unregister URL for debugging
    std::cout << "Unregister URL: " << ((_bSecureArrowheadInterface ? 
                                       SR_BASE_URI_HTTPS : SR_BASE_URI) + 
                                       "unregister?" + sParams) << std::endl;

    // Send request to appropriate endpoint
    if(_bSecureArrowheadInterface)
        return SendHttpsRequest("", SR_BASE_URI_HTTPS + "unregister?" + sParams, "DELETE");
    else
        return SendRequest("", SR_BASE_URI + "unregister?" + sParams, "DELETE");
}