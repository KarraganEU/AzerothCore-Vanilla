#include <boost/algorithm/string/find.hpp>
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
#include <GroupReference.h>
#include <Group.h>
#include <bot_ai.h>
#include <json.hpp>
#include <Player.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class BotSession : public std::enable_shared_from_this<BotSession> {
    //... other members as before ...
    tcp::resolver resolver_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    tcp::socket socket_;
    std::function<void(http::response<http::string_body>)> callback_;

public:
    explicit BotSession(net::io_context& ioc)
        : resolver_(net::make_strand(ioc)), socket_(net::make_strand(ioc)) {}

    void run(char const* host, char const* port, char const* target, int version, const std::string& body, std::function<void(http::response<http::string_body>)> callback) {
        callback_ = std::move(callback);
        //req_.version(version);
        req_.method(http::verb::post);
        req_.target(target);
        req_.body() = body;
        req_.set(http::field::host, host);        
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req_.set(http::field::content_type, "application/json");
        req_.content_length(body.size());

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

BotChatHandler* BotChatHandler::instance()
{
    static BotChatHandler instance;
    return &instance;
}

void BotChatHandler::handlePartyMessage(const std::string& message, Group& group)
{
    if (message.find(itemlinkToken) != std::string::npos) {
        parseResult parseResult = parseItemLink(message);
        for (const GroupBotReference* itr = group.GetFirstBotMember(); itr != nullptr; itr = itr->next())
        {
            Creature const* bot = itr->GetSource();
            if (!bot)
                continue;
            bot->GetBotAI()->handleChatItemLink(parseResult);
        }
        return;
    }

    //handle party commands
    if (message.find("listeq") != std::string::npos) {
        //list equipment        
        for (const GroupBotReference* itr = group.GetFirstBotMember(); itr != nullptr; itr = itr->next())
        {
            Creature const* bot = itr->GetSource();
            if (!bot)
                continue;
            bot->GetBotAI()->whisperEquipmentList(nullptr);
        }
        return;
    }
    //do proper chat
    std::map<std::string, const bot_ai*> botMap;
    for (const GroupBotReference* itr = group.GetFirstBotMember(); itr != nullptr; itr = itr->next())
    {
        Creature const* bot = itr->GetSource();
        if (!bot)
            continue;
        botMap.emplace(bot->GetName(), bot->GetBotAI());
    }

    json context = buildGroupContext(message, group);
    auto const host = "127.0.0.1";
    auto const port = "5000";
    auto const target = "/group/1234";
    int version = 1;

    net::io_context ioc;
    

    std::make_shared<BotSession>(ioc)->run(host, port, target, version, context.dump(), [&botMap](http::response<http::string_body> res) {

        std::cout << res.body() << std::endl;
        json replies = json::parse(res.body());
        for (auto const& rep : replies["replies"]) {
            auto it = botMap.find(rep["speaker"]);
            if (it == botMap.end()) {
                // Key not found
                continue;
            }
            const bot_ai* speaker = it->second;
            speaker->BotTellParty(rep["message"], nullptr);
        }
    });
        

    ioc.run();
}

//TODO maybe add caching?
json BotChatHandler::buildGroupContext(const std::string& message, Group& group) // group "can't" be const, because getLeader is not const and would require extra coding
{
    json postBody;
    postBody["string"] = message;
    postBody["context"] = json::object();   
    postBody["context"]["players"] = json::array();
    postBody["context"]["bots"] = json::array();
    //add player context
    const Player* player = group.GetLeader();
    json leader;
    leader["name"] = player->GetName();
    leader["level"] = player->GetLevel();
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(player->getRace());
    leader["race"] = rEntry->name[0]; 
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(player->getClass());
    leader["class"] = cEntry->name[0];
    leader["gender"] = player->getGender() != 0 ? "female" : "male";
    //TODO How the fuck do I get the Player spec as string
    postBody["context"]["players"].push_back(leader);

    //add bot context
    for (const GroupBotReference* itr = group.GetFirstBotMember(); itr != nullptr; itr = itr->next())
    {
        Creature const* bot = itr->GetSource();
        if (!bot)
            continue;
        json botJSON;
        botJSON["name"] = bot->GetName();
        botJSON["level"] = bot->GetLevel();
        ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(bot->getRace());
        botJSON["race"] = rEntry->name[0];
        ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(bot->getClass());
        botJSON["class"] = cEntry->name[0];
        botJSON["spec"] = bot_ai::LocalizedNpcText(player, bot->GetBotAI()->TextForSpec(bot->GetBotAI()->GetSpec()));
        botJSON["gender"] = bot->getGender() != 0 ? "female" : "male";
        postBody["context"]["bots"].push_back(botJSON);
    }    
    return postBody;
}

BotChatHandler::parseResult BotChatHandler::parseItemLink(const std::string& message)
{
    //Check for Linked Item
    //Item links have this kind of format: "\124cff0070dd\124Hitem:13042::::::::80:::::\124h[Sword of the Magistrate]\124h\124r", where the first block signifies color, and the part after 'Hitem:' is the itemID    
    std::string token = "|Hitem:";
    std::string link = message;

    std::size_t pos = link.find(token);
    parseResult res;
    if (pos != std::string::npos) {
        //get itemTemplate by itemId
        link = link.substr(pos + token.length());
        pos = link.find(":");
        res.proto = sObjectMgr->GetItemTemplate(std::stoi(link.substr(0, pos)));

        //find suffix data - like shadow wrath, monkey, gorilla, etc
        boost::iterator_range<std::string::iterator> suffixIdItr = boost::find_nth(link, ":", 5);
        boost::iterator_range<std::string::iterator> suffixFactorItr = boost::find_nth(link, ":", 6);
        boost::iterator_range<std::string::iterator> suffixEndItr = boost::find_nth(link, ":", 7);
        if (suffixFactorItr && suffixIdItr && suffixEndItr) {
            ptrdiff_t idIdx = std::distance(link.begin(), suffixIdItr.end());
            ptrdiff_t factorIdx = std::distance(link.begin(), suffixFactorItr.end());
            ptrdiff_t endIdx = std::distance(link.begin(), suffixEndItr.end());
            std::string suffixId = link.substr(idIdx, factorIdx - (idIdx + 1));
            std::string suffixFactor = link.substr(factorIdx, endIdx - (factorIdx + 1));
            res.suffixFactor = std::stoi(suffixFactor);
            res.suffixId = std::abs(std::stoi(suffixId));
        }

    }
    return res;
}

BotChatHandler::BotChatHandler()
{
}
