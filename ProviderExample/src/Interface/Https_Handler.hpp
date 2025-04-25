// Https_Handler.hpp
#pragma once
#include <string>

using namespace std;

// Keep the defines for compatibility, but they won't be used
#define SERVERKEYFILE	"keys2/tempsensor.testcloud2.privkey.pem"
#define SERVERCERTFILE	"keys2/tempsensor.testcloud2.clcert.pem"
#define ROOTCACERTFILE  "keys2/tempsensor.testcloud2.caCert.pem"

class Https_Handler {
private:
    void* pmhd = NULL;  // Using void* instead of MHD-specific type

public:
    Https_Handler() = default;
    virtual ~Https_Handler() = default;
    
    // Stub client method
    int SendHttpsRequest(string pdata, string paddr, string pmethod);
    
    // Stub server methods
    int MakeHttpsServer(unsigned short listen_port);
    int KillHttpsServer();
    
    // Callbacks (kept for compatibility)
    virtual int httpsGETCallback(const char *Id, string *pData_str, string sToken, string sSignature, string clientDistName);
    virtual size_t httpsResponseCallback(char *ptr, size_t size);
};