// Http_Handler.cpp
#include "Http_Handler.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <thread>

// using tcp = boost::asio::ip::tcp;
// namespace http = boost::beast::http;
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
         // ----------- Check connection with Arrowhead /echo -----------
         std::string test_host = "172.25.164.36";
         std::string test_port = "8443";
         std::string test_target = "/serviceregistry/echo";
 
         net::io_context ioc;
         tcp::resolver resolver(ioc);
         beast::tcp_stream stream(ioc);
 
         auto const results = resolver.resolve(test_host, test_port);
         stream.connect(results);
 
         http::request<http::string_body> test_req{http::verb::get, test_target, 11};
         test_req.set(http::field::host, test_host);
         test_req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
 
         http::write(stream, test_req);
 
         beast::flat_buffer buffer;
         http::response<http::string_body> test_res;
         http::read(stream, buffer, test_res);
 
         if (test_res.result() != http::status::ok) {
             std::cerr << "[ERROR] Arrowhead registry echo failed with status: " << test_res.result_int() << "\n";
             return 500;
         }
 
         std::cout << "[INFO] Arrowhead /echo responded: " << test_res.body() << "\n";
 
         // Gracefully close the socket
         beast::error_code ec;
         stream.socket().shutdown(tcp::socket::shutdown_both, ec);
 
         // Ignore not_connected errors
         if (ec && ec != beast::errc::not_connected)
             throw beast::system_error{ec};
 
     } catch (const std::exception& e) {
         std::cerr << "[ERROR] Failed to contact Arrowhead registry /echo: " << e.what() << "\n";
         return 500;
     }
 
     // ----------- Then continue with your actual POST/PUT request -----------
     std::cout << "[DEBUG] Proceeding with actual SendRequest logic to: " << paddr << "\n";
    try {
        auto const pos = paddr.find("://");
        auto const host_start = (pos == std::string::npos) ? 0 : pos + 3;
        auto const host_end = paddr.find('/', host_start);
        std::string host = paddr.substr(host_start, host_end - host_start);
        std::string target = paddr.substr(host_end);

        int version = 11;  // HTTP/1.1

        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host, "80");

        tcp::socket socket(ioc_);
        boost::asio::connect(socket, results.begin(), results.end());

        http::request<http::string_body> req;
        req.version(version);
        req.method_string(pmethod);
        req.target(target);
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = pdata;
        req.prepare_payload();

        http::write(socket, req);

        http::response<http::string_body> res;
        boost::beast::flat_buffer buffer;
        http::read(socket, buffer, res);

        std::cout << res << std::endl;

        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        return 200;
    } catch (std::exception& e) {
        std::cerr << "Error in SendRequest: " << e.what() << std::endl;
        return 500;  // internal server error
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
    *pData_str = R"({"message":"Hello from "})" + std::string(Id);
    return 1;
}

std::size_t Http_Handler::httpResponseCallback(char* ptr, std::size_t size) {
    (void)ptr;
    return size;
}
