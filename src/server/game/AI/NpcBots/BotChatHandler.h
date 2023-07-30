#ifndef BOT_CHATHANDLER_H_
#define BOT_CHATHANDLER_H_

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/thread/thread.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <json.hpp>
#include <Chat.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class BotChatHandler
{
public:
private:
    inline static const std::string _itemlinkToken {"|Hitem:"};
    bool _enableChat;
    std::string _host;
    std::string _port;
    std::string _groupTarget;
    bool _enableSay;
    bool _enableParty;
    bool _enableSpecGear;
    boost::asio::io_context ioc;
    boost::thread_group threadPool;
    boost::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;
    std::size_t poolSize;

public:
    static BotChatHandler* instance();

    BotChatHandler(BotChatHandler const&) = delete; // Prevent copying
    void operator=(BotChatHandler const&) = delete; // Prevent assignment
    BotChatHandler() = delete;
    ~BotChatHandler() {
        work_guard.reset();
        threadPool.join_all();
    }

    void queryBotReply(std::string body, std::map<std::string, const bot_ai*>& botMap, uint64 leaderId);
    void handlePartyMessage(const std::string& message, Group& group);
    void setPartyMode(const std::string& mode, uint64 leaderId, ChatHandler* handler);
    void eraseHistory(uint64 leaderId, ChatHandler* handler);
    json buildGroupContext(const std::string& message, Group& group);
    struct parseResult {
        ItemTemplate const* proto = 0;
        uint32 suffixId = 0;
        uint32 suffixFactor = 0;
    };
    parseResult parseItemLink(const std::string& message);


private:
    BotChatHandler(std::size_t size) : poolSize(size), work_guard(boost::asio::make_work_guard(ioc)) {
        //make poolsize configurable
        loadConfig();
        start();
    }
    void loadConfig();
    void start();
    std::string getSpecName(uint32 spec);
    
};

#define sBotChatHandler BotChatHandler::instance()

#endif
