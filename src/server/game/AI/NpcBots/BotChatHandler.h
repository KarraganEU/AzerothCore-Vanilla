#ifndef BOT_CHATHANDLER_H_
#define BOT_CHATHANDLER_H_

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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class BotChatHandler
{
public:
    static BotChatHandler* instance();
private:
    BotChatHandler();
    //~BotChatHandler();
};

//class BotChatRequestSession : public std::enable_shared_from_this<BotChatRequestSession> {
//    tcp::resolver resolver_;
//    beast::flat_buffer buffer_;
//    http::request<http::empty_body> req_;
//    http::response<http::string_body> res_;
//    tcp::socket socket_;
//
//public:
//    explicit BotChatRequestSession(net::io_context& ioc)
//        : resolver_(net::make_strand(ioc)), socket_(net::make_strand(ioc)) {}
//    void run(char const* host, char const* port, char const* target, int version, std::function<void(http::response<http::string_body>)> callback);
//    void on_resolve(beast::error_code ec, tcp::resolver::results_type results, std::function<void(http::response<http::string_body>)> callback);
//    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type, std::function<void(http::response<http::string_body>)> callback);
//    void on_write(beast::error_code ec, std::size_t bytes_transferred, std::function<void(http::response<http::string_body>)> callback);
//    void on_read(beast::error_code ec, std::size_t bytes_transferred, std::function<void(http::response<http::string_body>)> callback);
//    void fail(beast::error_code ec, char const* what);
//    static void test() {
//        auto const host = "localhost";
//        auto const port = "5000";
//        auto const target = "/test";
//        int version = 1;
//
//        net::io_context ioc;
//
//        std::make_shared<BotChatRequestSession>(ioc)->run(host, port, target, version, [](http::response<http::string_body> res) {
//            std::cout << res << std::endl;
//            });
//
//        ioc.run();
//    };
//};

#define sBotChatHandler BotChatHandler::instance()

#endif
