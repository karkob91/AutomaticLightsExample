// Http_Handler.cpp
#include "Http_Handler.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

Http_Handler::Http_Handler()
    : acceptor_(ioc_)
{}

Http_Handler::~Http_Handler() {
    KillServer();
}

int Http_Handler::SendRequest(const std::string& pdata, const std::string& paddr, const std::string& pmethod) {
     try {
         // Parse URL to extract components
         std::string host, port, target;
         
         // Extract protocol, host, port and path
         auto const protocol_pos = paddr.find("://");
         auto const host_start = (protocol_pos == std::string::npos) ? 0 : protocol_pos + 3;
         auto const colon_pos = paddr.find(':', host_start);
         auto const path_start = paddr.find('/', host_start);
         
         if (colon_pos != std::string::npos && (path_start == std::string::npos || colon_pos < path_start)) {
             // URL has port specification
             host = paddr.substr(host_start, colon_pos - host_start);
             auto const port_end = (path_start == std::string::npos) ? paddr.length() : path_start;
             port = paddr.substr(colon_pos + 1, port_end - (colon_pos + 1));
         } else {
             // No port in URL, use default port based on protocol
             host = (path_start == std::string::npos) ? 
                 paddr.substr(host_start) : 
                 paddr.substr(host_start, path_start - host_start);
                 
             // Default port based on protocol
             if (protocol_pos != std::string::npos && paddr.substr(0, protocol_pos) == "https") {
                 port = "443";
             } else {
                 port = "80";
             }
         }
         
         target = (path_start == std::string::npos) ? "/" : paddr.substr(path_start);
         
         std::cout << "[DEBUG] Parsed URL - Host: " << host << ", Port: " << port << ", Target: " << target << std::endl;
         
         // Setup Boost.Beast components
         net::io_context ioc;
         tcp::resolver resolver(ioc);
         beast::tcp_stream stream(ioc);
         
         // Use explicit IP address instead of hostname resolution when possible
         // This helps avoid DNS resolution issues
         if (host == "172.25.164.36") {
             // Using direct IP and port
             tcp::endpoint endpoint(net::ip::make_address(host), std::stoi(port));
             stream.connect(endpoint);
         } else {
             // Standard resolution
             auto const results = resolver.resolve(host, port);
             stream.connect(results);
         }
         
         // Determine the HTTP verb from the method string
         http::verb verb;
         if (pmethod == "GET") verb = http::verb::get;
         else if (pmethod == "POST") verb = http::verb::post;
         else if (pmethod == "PUT") verb = http::verb::put;
         else if (pmethod == "DELETE") verb = http::verb::delete_;
         else throw std::runtime_error("Unsupported HTTP method: " + pmethod);
         
         // Create the request
         http::request<http::string_body> req{verb, target, 11};
         req.set(http::field::host, host);
         req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
         
         // Set appropriate headers and body
         if (!pdata.empty()) {
             req.set(http::field::content_type, "application/json");
             req.body() = pdata;
             req.prepare_payload();
         }
         
         // Log the final request for debugging
         std::cout << "[DEBUG] Final composed HTTP request:\n" << req << std::endl;
         
         // Send the request
         http::write(stream, req);
         
         // Receive the response
         beast::flat_buffer buffer;
         http::response<http::string_body> res;
         http::read(stream, buffer, res);
         
         // Log the response
         std::cout << "[INFO] Response status: " << res.result_int() << std::endl;
         std::cout << res.body() << std::endl;
         
         // Clean up
         beast::error_code ec;
         stream.socket().shutdown(tcp::socket::shutdown_both, ec);
         if (ec && ec != beast::errc::not_connected)
             throw beast::system_error{ec};
         
         return res.result_int();
     }
     catch (std::exception& e) {
         std::cerr << "Error in SendRequest: " << e.what() << std::endl;
         return 500;
     }
 }
int Http_Handler::MakeServer(unsigned short listen_port) {
    try {
        tcp::endpoint endpoint(tcp::v4(), listen_port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        server_running_ = true;
        server_thread_ = std::make_unique<std::thread>([this]() {
            doAccept();
            ioc_.run();
        });

        std::cout << "Server listening on port " << listen_port << std::endl;
        return 0;
    } catch (std::exception& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        return 1;
    }
}

int Http_Handler::KillServer() {
    if (server_running_) {
        ioc_.stop();
        if (server_thread_ && server_thread_->joinable()) {
            server_thread_->join();
        }
        server_running_ = false;
        std::cout << "Server stopped." << std::endl;
        return 0;
    }
    return 1;
}

void Http_Handler::doAccept() {
    auto socket = std::make_shared<tcp::socket>(ioc_);
    acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (!ec) handleSession(socket);
        if (server_running_) doAccept();
    });
}

void Http_Handler::handleSession(std::shared_ptr<tcp::socket> socket) {
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    auto req = std::make_shared<http::request<http::string_body>>();

    http::async_read(*socket, *buffer, *req,
        [this, socket, buffer, req](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                std::string response_body;
                http::status status_code = http::status::ok;

                if (req->method() == http::verb::get) {
                    std::string target_str(req->target());
                    int result = httpGETCallback(target_str.c_str(), &response_body);
                    if (!result) {
                        status_code = http::status::internal_server_error;
                        response_body = "{\"error\":\"Callback failed\"}";
                    }
                } else {
                    status_code = http::status::method_not_allowed;
                    response_body = "{\"error\":\"Only GET allowed\"}";
                }

                http::response<http::string_body> res{status_code, req->version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req->keep_alive());
                res.body() = response_body;
                res.prepare_payload();

                http::async_write(*socket, res, [socket](boost::system::error_code, std::size_t) {});
            }
        });
}

int Http_Handler::httpGETCallback(const char* Id, std::string* pData_str) {
    *pData_str = R"({\"message\":\"Hello from \"})" + std::string(Id);
    return 1;
}

std::size_t Http_Handler::httpResponseCallback(char* ptr, std::size_t size) {
    (void)ptr;
    return size;
}
