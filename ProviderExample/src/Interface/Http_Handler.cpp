// Http_Handler.cpp
#include "Http_Handler.hpp"
#include <iostream>

int Http_Handler::SendRequest(string pdata, string paddr, string pmethod) {
    std::cout << "Sending " << pmethod << " request to: " << paddr << std::endl;
    
    // For POST and PUT, print the data
    if (pmethod == "POST" || pmethod == "PUT") {
        std::cout << "Request data: " << pdata << std::endl;
    }
    
    // For demonstration, return success
    return 200;  // HTTP OK
}

int Http_Handler::MakeServer(unsigned short listen_port) {
    std::cout << "Starting HTTP server on port " << listen_port << std::endl;
    
    // Simulate server creation
    pmhd = (void*)1;  // Just a non-NULL pointer
    return 0;  // Success
}

int Http_Handler::KillServer() {
    if (pmhd) {
        std::cout << "Stopping HTTP server" << std::endl;
        pmhd = NULL;
        return 0;  // Success
    }
    return 1;  // Error
}

int Http_Handler::httpGETCallback(const char *Id, string *pData_str) {
    std::cout << "HTTP GET request for path: " << Id << std::endl;
    *pData_str = "1234";  // Default response
    return 1;  // Success
}

size_t Http_Handler::httpResponseCallback(char *ptr, size_t size) {
    return size;  // Simply return the size
}

// External callback function
size_t httpResponseHandler(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return ((Http_Handler *)userdata)->httpResponseCallback(ptr, size*nmemb);
}