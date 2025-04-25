// Http_Handler.hpp
#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <functional>
#include <memory>

class Http_Handler {
public:
    Http_Handler();
    ~Http_Handler();

    // HTTP client (async)
    int SendRequest(const std::string& pdata, const std::string& paddr, const std::string& pmethod);


    // HTTP server (async)
    int MakeServer(unsigned short listen_port);
    int KillServer();

    // Callbacks
    virtual int httpGETCallback(const char* Id, std::string* pData_str);
    virtual std::size_t httpResponseCallback(char* ptr, std::size_t size);

private:
    // Client-side
    void onClientResolve(const boost::system::error_code&, boost::asio::ip::tcp::resolver::results_type);
    void onClientConnect(const boost::system::error_code&);
    void onClientWrite(const boost::system::error_code&, std::size_t);
    void onClientRead(const boost::system::error_code&, std::size_t);

    // Server-side helpers
    void doAccept();
    void handleSession(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    // I/O
    boost::asio::io_context ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unique_ptr<std::thread> server_thread_;
    bool server_running_ = false;

    // Request/response buffers
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;

    std::string outgoing_data_;
};
