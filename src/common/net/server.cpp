#include <fmt/format.h>

#include <algorithm>
#include <common/logger.hpp>
#include <common/net/server.hpp>
#include <iterator>  // for std::back_inserter
#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string_view>

#include "common/net/game_packet_server.hpp"

using namespace familyline::net;
using json = nlohmann::json;

#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

using namespace curlpp::options;

std::stringstream CServer::buildRequest(
    curlpp::Easy& req, std::string endpoint, std::string method, bool jsonbody, std::string data)
{
    std::stringstream outstr;

    // Set the URL and the timeout lue.
    req.setOpt<Url>(fmt::format("http://{}/{}", http_address_, endpoint));
    req.setOpt<Timeout>(timeout_secs_);
    req.setOpt<WriteStream>(&outstr);

    if (method == "POST") {
        req.setOpt<Post>(true);
    }
    if (method == "PUT") {
        req.setOpt<CustomRequest>("PUT");
    }

    if (jsonbody) {
        std::list<std::string> header;
        auto datasize = data.size();

        header.push_back("Accept: application/json");
        header.push_back("Content-Type: application/json");
        req.setOpt<HttpHeader>(header);
        req.setOpt<PostFields>(data);
        req.setOpt<PostFieldSize>(data.size());
    }

    return outstr;
}

uint64_t CServer::getUserID() const { return cci_.has_value() ? cci_->info.id : 0; }
bool CServer::isLogged() const { return cci_.has_value(); }
std::string CServer::getAddress() const
{
    if (isLogged()) return http_address_;

    return "";
}

NetResult CServer::checkErrors(unsigned httpcode, std::stringstream& body)
{
    body.seekg(0);

    auto& log = LoggerService::getLogger();

    if (httpcode < 200 || httpcode > 299) {
        switch (httpcode) {
            case 400: {
                json response       = json::parse(body);
                std::string message = response["message"];
                log->write("cli-server", LogType::Error, "login failure: %s", message.c_str());
                if (message.find("Not all clients are ready") != std::string::npos) {
                    return NetResult::NotAllClientsConnected;
                }

                return NetResult::LoginFailure;
            }
            default:
                log->write(
                    "cli-server", LogType::Error, "login returned weird HTTP code: %d", httpcode);
                return NetResult::ConnectionError;
        }
    }

    body.seekg(0);
    return NetResult::OK;
}

NetResult CServer::login(std::string address, std::string username)
{
    http_address_ = address;
    auto& log     = LoggerService::getLogger();

    log->write("cli-server", LogType::Info, "logging to %s", address.c_str());
    unsigned httpcode = 0;

    try {
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup myCleanup;

        // Our request to be sent.
        curlpp::Easy myRequest;

        json j;
        j["name"] = username;
        auto sstr = this->buildRequest(myRequest, "login", "POST", true, j.dump());

        // Send request and get a result.
        // By default the result goes to standard output.
        myRequest.perform();

        auto httpcode = curlpp::infos::ResponseCode::get(myRequest);

        if (auto e = this->checkErrors(httpcode, sstr); e != NetResult::OK) {
            return e;
        }

        json response     = json::parse(sstr);
        std::string token = response["token"];
        uint64_t userid   = response["user_id"];

        if (response["name"] != username || token == "" || userid == 0) {
            return NetResult::ServerError;
        }

        cci_ = std::optional(CurrentClientInfo{CClientInfo{userid, username, false}, token});

        log->write(
            "cli-server", LogType::Info,
            "logged succesfully: username=%s, userid=%ull, token=<XXX>", username.c_str(), userid);

    } catch (curlpp::RuntimeError& e) {
        std::string_view exc{e.what()};
        log->write("cli-server", LogType::Error, "CURL error: %s,", exc.data());

        if (exc.find("timed out") != std::string::npos) {
            return NetResult::ConnectionTimeout;
        }

        if (exc.find("Could not resolve host") != std::string::npos) {
            return NetResult::ConnectionError;
        }

        if (exc.find("Connection refused") != std::string::npos) {
            return NetResult::ConnectionError;
        }
        
        if (exc.find("reset by peer") != std::string::npos) {
            return NetResult::ConnectionError;
        }

        return NetResult::ServerError;

    } catch (curlpp::LogicError& e) {
        std::cerr << "Logic: " << e.what() << std::endl;
    } catch (nlohmann::detail::exception& e) {
        log->write(
            "cli-server", LogType::Error,
            "server returned status %d and we could not parse the incoming JSON (%s),", httpcode,
            e.what());
        return NetResult::ServerError;
    }

    return NetResult::OK;
}

json createTokenMessage(std::string token)
{
    json j;
    j["token"] = token;
    return j;
}

/**
 * Logout from the server
 *
 * This function should not fail, but, like the real life, where some retarded
 * people might not let you leave a certain place because 'they want to stay with you',
 * the server might act retarded.
 * Or the server might crash.
 *
 * You must be prepared for that.
 */
NetResult CServer::logout()
{
    auto& log = LoggerService::getLogger();

    if (!isLogged()) {
        log->write("cli-server", LogType::Warning, "logging out but already logged off");
        return NetResult::AlreadyLoggedOff;
    }

    log->write("cli-server", LogType::Info, "logging out from %s", http_address_.c_str());
    unsigned httpcode = 0;

    try {
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup myCleanup;

        // Our request to be sent.
        curlpp::Easy myRequest;

        json j    = createTokenMessage(cci_->token);
        auto sstr = this->buildRequest(myRequest, "logout", "POST", true, j.dump());

        // Send request and get a result.
        // By default the result goes to standard output.
        myRequest.perform();

        auto httpcode = curlpp::infos::ResponseCode::get(myRequest);

        if (auto e = this->checkErrors(httpcode, sstr); e != NetResult::OK) {
            return e;
        }

        json response = json::parse(sstr);
        auto serverid = response["id"];

        if (serverid != cci_->info.id) {
            log->write(
                "cli-server", LogType::Error,
                "for some reason, IDs between client and server differ (cli %llx != ser %llx)",
                cci_->info.id, (uint64_t)serverid);
            return NetResult::ServerError;
        }

        log->write("cli-server", LogType::Info, "logged out succesfully");

    } catch (curlpp::RuntimeError& e) {
        std::string_view exc{e.what()};
        if (exc.find("Connection timed out") != std::string::npos) {
            return NetResult::ConnectionTimeout;
        }
    } catch (curlpp::LogicError& e) {
        std::cerr << "Logic: " << e.what() << std::endl;
    } catch (nlohmann::detail::exception& e) {
        log->write(
            "cli-server", LogType::Error,
            "server returned status %d and we could not parse the incoming JSON (%s),", httpcode,
            e.what());
        return NetResult::ServerError;
    }

    return NetResult::OK;
}

NetResult CServer::getServerInfo(CServerInfo& info)
{
    auto& log = LoggerService::getLogger();

    if (!isLogged()) {
        log->write("cli-server", LogType::Warning, "getting server info but already logged off");
        return NetResult::AlreadyLoggedOff;
    }

    log->write("cli-server", LogType::Info, "getting server info from %s", http_address_.c_str());
    unsigned httpcode = 0;

    try {
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup myCleanup;

        // Our request to be sent.
        curlpp::Easy myRequest;

        json j    = createTokenMessage(cci_->token);
        auto sstr = this->buildRequest(myRequest, "info", "POST", true, j.dump());

        // Send request and get a result.
        // By default the result goes to standard output.
        myRequest.perform();

        auto httpcode = curlpp::infos::ResponseCode::get(myRequest);

        if (auto e = this->checkErrors(httpcode, sstr); e != NetResult::OK) {
            return e;
        }

        json response    = json::parse(sstr);
        info.name        = response["name"];
        info.max_clients = response["max_clients"];

        auto& jclients = response["clients"];

        info.clients.clear();
        std::transform(
            std::begin(jclients), std::end(jclients), std::back_inserter(info.clients),
            [&](auto& jclient) {
                return CClientInfo{jclient["user_id"], jclient["name"], jclient["ready"]};
            });

        info_ = info;

    } catch (curlpp::RuntimeError& e) {
        std::string_view exc{e.what()};
        if (exc.find("Connection timed out") != std::string::npos) {
            return NetResult::ConnectionTimeout;
        }
    } catch (curlpp::LogicError& e) {
        std::cerr << "Logic: " << e.what() << std::endl;
    } catch (nlohmann::detail::exception& e) {
        log->write(
            "cli-server", LogType::Error,
            "server returned status %d and we could not parse the incoming JSON (%s),", httpcode,
            e.what());
        return NetResult::ServerError;
    }

    return NetResult::OK;
}

bool CServer::isReady() const { return cci_->info.ready; }

NetResult CServer::toggleReady(bool value)
{
    auto& log = LoggerService::getLogger();

    if (!isLogged()) {
        log->write("cli-server", LogType::Warning, "getting server info but already logged off");
        return NetResult::AlreadyLoggedOff;
    }

    log->write(
        "cli-server", LogType::Info, "setting ready from %s to %s", http_address_.c_str(),
        value ? "true" : "false");
    unsigned httpcode = 0;

    try {
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup myCleanup;

        // Our request to be sent.
        curlpp::Easy myRequest;

        json j               = createTokenMessage(cci_->token);
        std::string endpoint = fmt::format("ready/{}", value ? "set" : "unset");

        auto sstr = this->buildRequest(myRequest, endpoint, "PUT", true, j.dump());

        // Send request and get a result.
        // By default the result goes to standard output.
        myRequest.perform();

        auto httpcode = curlpp::infos::ResponseCode::get(myRequest);

        if (auto e = this->checkErrors(httpcode, sstr); e != NetResult::OK) {
            return e;
        }
        cci_->info.ready = value;

    } catch (curlpp::RuntimeError& e) {
        std::string_view exc{e.what()};
        if (exc.find("Connection timed out") != std::string::npos) {
            return NetResult::ConnectionTimeout;
        }
    } catch (curlpp::LogicError& e) {
        std::cerr << "Logic: " << e.what() << std::endl;
    } catch (nlohmann::detail::exception& e) {
        log->write(
            "cli-server", LogType::Error,
            "server returned status %d and we could not parse the incoming JSON (%s),", httpcode,
            e.what());
        return NetResult::ServerError;
    }

    return NetResult::OK;
}

bool CServer::isConnecting() const { return (gsi_->address != "" && gsi_->port > 0); }

NetResult CServer::connect()
{
    auto& log = LoggerService::getLogger();

    if (!isLogged()) {
        log->write("cli-server", LogType::Warning, "getting server info but already logged off");
        return NetResult::AlreadyLoggedOff;
    }

    log->write("cli-server", LogType::Info, "connecting to %s", http_address_.c_str());
    unsigned httpcode = 0;

    try {
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup myCleanup;

        // Our request to be sent.
        curlpp::Easy myRequest;

        json j    = createTokenMessage(cci_->token);
        auto sstr = this->buildRequest(myRequest, "connect", "POST", true, j.dump());

        // Send request and get a result.
        // By default the result goes to standard output.
        myRequest.perform();

        auto httpcode = curlpp::infos::ResponseCode::get(myRequest);

        if (auto e = this->checkErrors(httpcode, sstr); e != NetResult::OK) {
            return e;
        }

        json response       = json::parse(sstr);
        std::string address = response["address"];
        int port            = response["port"];

        gsi_ = std::optional(GameServerInfo{address, port});

        log->write(
            "cli-server", LogType::Info, "game data transmission server at %s:%d", address.c_str(),
            port);

    } catch (curlpp::RuntimeError& e) {
        std::string_view exc{e.what()};
        if (exc.find("Connection timed out") != std::string::npos) {
            return NetResult::ConnectionTimeout;
        }
    } catch (curlpp::LogicError& e) {
        std::cerr << "Logic: " << e.what() << std::endl;
    } catch (nlohmann::detail::exception& e) {
        log->write(
            "cli-server", LogType::Error,
            "server returned status %d and we could not parse the incoming JSON (%s),", httpcode,
            e.what());
        return NetResult::ServerError;
    }

    return NetResult::OK;
}

std::optional<GamePacketServer> CServer::getGameServer()
{
    if (!this->isLogged() || !this->isConnecting()) {
        return std::nullopt;
    }

    return std::make_optional<GamePacketServer>(
        gsi_->address, gsi_->port, cci_->token, cci_->info.id, info_.clients);
}
