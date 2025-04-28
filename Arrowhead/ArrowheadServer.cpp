#include "ArrowheadServer.h"
#include <iostream>
#include <memory>

// Forward declaration for HTTP session class
class ArrowheadServer::HttpSession : public std::enable_shared_from_this<ArrowheadServer::HttpSession> {
public:
    // Construct with a socket
    HttpSession(tcp::socket socket, ArrowheadServer& server)
        : stream_(std::move(socket)), server_(server) {}
    // Start the session
    void start() {
        readRequest();
    }

private:
    // The socket for this session
    beast::tcp_stream stream_;
    
    // Reference to the ArrowheadServer that created this session
    ArrowheadServer& server_;
    
    // Buffer for reading
    beast::flat_buffer buffer_{8192};
    
    // The request
    http::request<http::string_body> req_;
    
    // The response
    http::response<http::string_body> res_;
    
    // Read a request
    void readRequest() {
        auto self = shared_from_this();
        
        // Set timeout for reading
        stream_.expires_after(std::chrono::seconds(30));
        
        // Read a request
        http::async_read(stream_, buffer_, req_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                
                if (ec) {
                    if (ec != http::error::end_of_stream)
                        std::cerr << "Error reading: " << ec.message() << std::endl;
                    return;
                }
                
                self->handleRequest();
            });
    }
    
    // Handle the request
    void handleRequest() {
        // Set up the response
        res_.version(req_.version());
        res_.keep_alive(req_.keep_alive());
        
        // Let the ArrowheadServer handle the request
        server_.handleRequest(req_, res_);
        
        // Send the response
        sendResponse();
    }
    
    // Send the response
    void sendResponse() {
        auto self = shared_from_this();
        
        // Set timeout for writing
        stream_.expires_after(std::chrono::seconds(30));
        
        // Send the response
        http::async_write(stream_, res_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                
                if (ec) {
                    std::cerr << "Error writing: " << ec.message() << std::endl;
                    return;
                }
                
                // If we're not keeping alive, close the connection
                if (!self->res_.keep_alive()) {
                    self->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
                    return;
                }
                
                // Read another request
                self->req_ = {};
                self->readRequest();
            });
    }
};

ArrowheadServer::ArrowheadServer(unsigned short port)
    : port_(port), acceptor_(ioc_), running_(false)
{
    // Set up the default handler
    default_handler_ = [this](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
        notFound(res);
    };
}

ArrowheadServer::~ArrowheadServer() {
    stop();
}

void ArrowheadServer::on(const std::string& endpoint, const std::string& method, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    
    // Create a key from the endpoint and method
    std::string key = method + " " + endpoint;
    
    // Register the handler
    handlers_[key] = handler;
    
    std::cout << "Registered handler for " << key << std::endl;
}

bool ArrowheadServer::start() {
    if (running_) {
        return true; // Already running
    }
    
    try {
        // Set up the acceptor
        tcp::endpoint endpoint(tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
        
        // Start accepting connections
        running_ = true;
        server_thread_ = std::thread([this]() {
            doAccept();
            ioc_.run();
        });
        
        std::cout << "Server started on port " << port_ << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error starting server: " << e.what() << std::endl;
        return false;
    }
}

void ArrowheadServer::stop() {
    if (!running_) {
        return; // Already stopped
    }
    
    // Stop the io_context and wait for the thread to finish
    ioc_.stop();
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    running_ = false;
    std::cout << "Server stopped" << std::endl;
}

bool ArrowheadServer::isRunning() const {
    return running_;
}

void ArrowheadServer::doAccept() {
    // Create a socket
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                // Create a new session and start it
                std::make_shared<HttpSession>(std::move(socket), *this)->start();
            } else {
                std::cerr << "Error accepting connection: " << ec.message() << std::endl;
            }
            
            // Accept another connection
            if (running_) {
                doAccept();
            }
        });
}

void ArrowheadServer::handleRequest(const http::request<http::string_body>& req, 
                                  http::response<http::string_body>& res) {
    // Get the request method and target
    std::string method = req.method_string();
    std::string target = std::string(req.target());

    
    // Log the request
    std::cout << method << " " << target << std::endl;
    
    // Create a key from the method and target
    std::string key = method + " " + target;
    
    // Find the handler
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    auto it = handlers_.find(key);
    
    if (it != handlers_.end()) {
        // Call the handler
        it->second(req, res);
    } else {
        // Call the default handler
        default_handler_(req, res);
    }
}

void ArrowheadServer::notFound(http::response<http::string_body>& res) {
    res.result(http::status::not_found);
    res.set(http::field::content_type, "text/plain");
    res.body() = "404 Not Found";
    res.prepare_payload();
}