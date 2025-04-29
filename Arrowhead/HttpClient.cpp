#include "HttpClient.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace arrowhead {

// Pimpl implementation to hide Boost.Beast details
class HttpClient::Impl {
public:
    Impl() : timeout_(30) {}
    
    HttpResponse performRequest(
        http::verb method, 
        const std::string& protocol,
        const std::string& host,
        int port,
        const std::string& path,
        const std::string& query = "",
        const std::string& body = "",
        const std::string& contentType = "application/json") {
        
        HttpResponse response;
        
        try {
            // Set up Boost Beast for HTTP request
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            
            // Look up the domain name
            auto const results = resolver.resolve(host, std::to_string(port));
            
            // Make the connection
            stream.connect(results);
            
            // Set timeout
            stream.expires_after(std::chrono::seconds(timeout_));
            
            // Construct the target with path and query
            std::string target = path;
            if (!query.empty()) {
                target += "?" + query;
            }
            
            // Set up an HTTP request
            http::request<http::string_body> req{method, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            
            // Add custom headers
            for (const auto& header : headers_) {
                req.set(header.first, header.second);
            }
            
            // Add body if needed
            if (!body.empty()) {
                req.set(http::field::content_type, contentType);
                req.body() = body;
                req.prepare_payload();
            }
            
            // Log the full request if a callback is set
            if (logCallback_) {
                std::ostringstream oss;
                oss << req;
                logCallback_("Outgoing HTTP Request:\n" + oss.str());
            }
            
            // Send the HTTP request
            http::write(stream, req);
            
            // Buffer for reading
            beast::flat_buffer buffer;
            
            // Response object
            http::response<http::string_body> res;
            
            // Receive the HTTP response
            http::read(stream, buffer, res);
            
            // Log the full response if a callback is set
            if (logCallback_) {
                std::ostringstream oss;
                oss << res;
                logCallback_("Incoming HTTP Response:\n" + oss.str());
            }
            
            // Fill in the response struct
            response.statusCode = res.result_int();
            response.body = res.body();
            
            // Extract headers
            for (const auto& field : res) {
                response.headers[std::string(field.name_string())] = std::string(field.value());
            }
            
            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            
            if (ec && ec != beast::errc::not_connected && logCallback_) {
                logCallback_("Error closing connection: " + ec.message());
            }
        }
        catch (const std::exception& e) {
            if (logCallback_) {
                logCallback_("Error during HTTP request: " + std::string(e.what()));
            }
            response.statusCode = 0;
            response.body = "Error: " + std::string(e.what());
        }
        
        return response;
    }

    void setHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
    }
    
    void setTimeout(int seconds) {
        timeout_ = seconds;
    }
    
    void setLogCallback(std::function<void(const std::string&)> callback) {
        logCallback_ = std::move(callback);
    }
    
    std::string urlEncode(const std::string& str) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : str) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else if (c == ' ') {
                escaped << '+';
            } else {
                escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            }
        }

        return escaped.str();
    }

private:
    std::map<std::string, std::string> headers_;
    int timeout_;
    std::function<void(const std::string&)> logCallback_;
};

// Public implementation using the Pimpl pattern

HttpClient::HttpClient() : impl_(std::make_unique<Impl>()) {}

HttpClient::~HttpClient() = default;

HttpResponse HttpClient::get(
    const std::string& protocol, 
    const std::string& host, 
    int port, 
    const std::string& path,
    const std::string& query) {
    
    return impl_->performRequest(http::verb::get, protocol, host, port, path, query);
}

HttpResponse HttpClient::post(
    const std::string& protocol, 
    const std::string& host, 
    int port, 
    const std::string& path,
    const std::string& body,
    const std::string& contentType) {
    
    return impl_->performRequest(http::verb::post, protocol, host, port, path, "", body, contentType);
}

HttpResponse HttpClient::del(
    const std::string& protocol, 
    const std::string& host, 
    int port, 
    const std::string& path,
    const std::string& query) {
    
    return impl_->performRequest(http::verb::delete_, protocol, host, port, path, query);
}

void HttpClient::setHeader(const std::string& name, const std::string& value) {
    impl_->setHeader(name, value);
}

void HttpClient::setTimeout(int seconds) {
    impl_->setTimeout(seconds);
}

void HttpClient::setLogCallback(std::function<void(const std::string&)> callback) {
    impl_->setLogCallback(std::move(callback));
}

std::string HttpClient::urlEncode(const std::string& str) {
    return Impl().urlEncode(str);
}

} // namespace arrowhead