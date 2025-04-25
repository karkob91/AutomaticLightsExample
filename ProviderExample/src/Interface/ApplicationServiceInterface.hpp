#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Http_Handler.hpp"
#include "Https_Handler.hpp"

using json = nlohmann::json;
using namespace std;

typedef struct _Arrowhead_Data_ext
{
    string sServiceDefinition;
    string sserviceInterface;
    string sSystemName;
    string sServiceURI;
    map<string, string> vService_Meta;
    string sAuthenticationInfo;
} Arrowhead_Data_ext;

class ApplicationServiceInterface :
    Http_Handler,
    Https_Handler
{
private:
    // Replace dictionary with a map to store configuration
    map<string, string> configValues;

    string SR_BASE_URI;
    string SR_BASE_URI_HTTPS;
    string ADDRESS;
    string ADDRESS6;
    unsigned short PORT;
    string URI;
    string HTTPsURI;

    // New methods for loading/saving config
    bool LoadConfigFile(const string& filename);
    string GetConfigValue(const string& section, const string& key, const string& defaultValue);
    int GetConfigValueInt(const string& section, const string& key, int defaultValue);

public:
    ApplicationServiceInterface(string ini_file);
    ApplicationServiceInterface();
    ~ApplicationServiceInterface();

    bool init_ApplicationServiceInterface(string ini_file);
    int deinit();
    int registerToServiceRegistry(Arrowhead_Data_ext &stAH_data, bool _bSecureArrowheadInterface, bool _bProviderIsSecure);
    int unregisterFromServiceRegistry(Arrowhead_Data_ext &stAH_data, bool _bSecureArrowheadInterface, bool _bProviderIsSecure);

    int httpGETCallback(const char *Id, string *pData_str);
    int httpsGETCallback(const char *Id, string *pData_str, string param_token, string param_signature, string clientDistName);

    virtual int Callback_Serve_HTTP_GET(const char *Id, string *pData_str);
    virtual int Callback_Serve_HTTPs_GET(const char *Id, string *pData_str, string param_token, string param_signature, string clientDistName);
};