#pragma once

#include <string>
#include <functional>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
 * @brief HTTP request handler function type
 * 
 * This function type is used for handling HTTP requests.
 * @param request The HTTP request
 * @param response The HTTP response to be filled
 */
using RequestHandler = std::function<void(
    const http::request<http::string_body>&,
    http::response<http::string_body>&
)>;

/**
 * @brief Cross-platform HTTP server for Arrowhead service endpoints
 * 
 * This class provides a simple HTTP server for handling Arrowhead service endpoints.
 * It uses Boost.Beast for HTTP server functionality.
 */
class ArrowheadServer {
public:
    /**
     * @brief Construct a new Arrowhead Server object
     * 
     * @param port The port to listen on
     */
    ArrowheadServer(unsigned short port);
    
    /**
     * @brief Destroy the Arrowhead Server object
     */
    ~ArrowheadServer();
    
    /**
     * @brief Register a handler for a specific endpoint
     * 
     * @param endpoint The endpoint path (e.g., "/api_armed")
     * @param method The HTTP method (e.g., "GET", "POST")
     * @param handler The handler function
     */
    void on(const std::string& endpoint, const std::string& method, RequestHandler handler);
    
    /**
     * @brief Start the server
     * 
     * @return true if the server started successfully
     * @return false if the server failed to start
     */
    bool start();
    
    /**
     * @brief Stop the server
     */
    void stop();
    
    /**
     * @brief Check if the server is running
     * 
     * @return true if the server is running
     * @return false if the server is not running
     */
    bool isRunning() const;
    
private:
    unsigned short port_;
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::mutex handlers_mutex_;
    
    // Map from endpoint+method to handler
    std::map<std::string, RequestHandler> handlers_;
    
    // Default handler for unregistered endpoints
    RequestHandler default_handler_;
    
    // Session to handle a connection
    class HttpSession;
    
    // Accept incoming connections
    void doAccept();
    
    // Handle a request
    void handleRequest(const http::request<http::string_body>& req, 
                      http::response<http::string_body>& res);
    
    // Generate a simple 404 response
    void notFound(http::response<http::string_body>& res);
};