#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "BotChatHandler.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class BotSession : public std::enable_shared_from_this<BotSession> {
    //... other members as before ...
    tcp::resolver resolver_;
    beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;
    tcp::socket socket_;
    std::function<void(http::response<http::string_body>)> callback_;

public:
    explicit BotSession(net::io_context& ioc)
        : resolver_(net::make_strand(ioc)), socket_(net::make_strand(ioc)) {}

    void run(char const* host, char const* port, char const* target, int version, std::function<void(http::response<http::string_body>)> callback) {
        callback_ = std::move(callback);
        //req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(&BotSession::on_resolve, shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");

        beast::get_lowest_layer(socket_).async_connect(
            *results.begin(),
            beast::bind_front_handler(&BotSession::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec) {
        if (ec)
            return fail(ec, "connect");

        http::async_write(socket_, req_, beast::bind_front_handler(&BotSession::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        http::async_read(socket_, buffer_, res_, beast::bind_front_handler(&BotSession::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "read");

        callback_(std::move(res_));

        // Gracefully close the socket
        beast::error_code errc;
        socket_.shutdown(tcp::socket::shutdown_both, errc);

        if (ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");
    }
    void fail(beast::error_code ec, char const* what) {
        std::cerr << what << ": " << ec.message() << "\n";
    }
};


//int main(int argc, char** argv) {
//    auto const host = "www.example.com";
//    auto const port = "80";
//    auto const target = "/";
//    int version = 11;
//
//    net::io_context ioc;
//
//    std::make_shared<BotSession>(ioc)->run(host, port, target, version, [](http::response<http::string_body> res) {
//        std::cout << res << std::endl;
//        });
//
//    ioc.run();
//
//    return EXIT_SUCCESS;
//}

BotChatHandler* BotChatHandler::instance()
{
    return nullptr;
}
