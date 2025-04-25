// Http_Handler.hpp
#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

class Http_Handler {
private:
    void* pmhd = NULL;

public:
    Http_Handler() = default;
    virtual ~Http_Handler() = default;
    
    // Client method - simplified but still functional
    int SendRequest(string pdata, string paddr, string pmethod);
    
    // Server methods - simplified
    int MakeServer(unsigned short listen_port);
    int KillServer();
    
    // Callbacks
    virtual int httpGETCallback(const char *Id, string *pData_str);
    virtual size_t httpResponseCallback(char *ptr, size_t size);
};