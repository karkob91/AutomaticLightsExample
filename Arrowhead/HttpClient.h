#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>

namespace arrowhead {

/**
 * @brief HTTP response structure
 */
struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
};

/**
 * @brief Simple HTTP client class
 */
class HttpClient {
public:
    /**
     * @brief Default constructor
     */
    HttpClient();
    
    /**
     * @brief Destructor
     */
    ~HttpClient();
    
    /**
     * @brief Send an HTTP GET request
     * @param protocol The protocol (http or https)
     * @param host The hostname or IP address
     * @param port The port number
     * @param path The request path
     * @param query Optional query string
     * @return The HTTP response
     */
    HttpResponse get(const std::string& protocol, 
                    const std::string& host, 
                    int port, 
                    const std::string& path,
                    const std::string& query = "");
    
    /**
     * @brief Send an HTTP POST request
     * @param protocol The protocol (http or https)
     * @param host The hostname or IP address
     * @param port The port number
     * @param path The request path
     * @param body The request body
     * @param contentType The content type of the request body
     * @return The HTTP response
     */
    HttpResponse post(const std::string& protocol, 
                     const std::string& host, 
                     int port, 
                     const std::string& path,
                     const std::string& body,
                     const std::string& contentType = "application/json");
    
    /**
     * @brief Send an HTTP DELETE request
     * @param protocol The protocol (http or https)
     * @param host The hostname or IP address
     * @param port The port number
     * @param path The request path
     * @param query Optional query string
     * @return The HTTP response
     */
    HttpResponse del(const std::string& protocol, 
                    const std::string& host, 
                    int port, 
                    const std::string& path,
                    const std::string& query = "");
    
    /**
     * @brief Set a request header for all future requests
     * @param name The header name
     * @param value The header value
     */
    void setHeader(const std::string& name, const std::string& value);
    
    /**
     * @brief Set a timeout for requests in seconds
     * @param seconds The timeout in seconds
     */
    void setTimeout(int seconds);
    
    /**
     * @brief Set a callback for logging
     * @param callback The logging callback function
     */
    void setLogCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief URL encode a string
     * @param str The string to encode
     * @return The URL encoded string
     */
    static std::string urlEncode(const std::string& str);

private:
    // Internal implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace arrowhead