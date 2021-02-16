#pragma once

/**
 * Represents the server, from the client's point of view
 *
 * Copyright (C) 2021 Arthur Mendes
 */

#include <string>

#ifndef WIN32
#define SOCKET int
#endif

#ifdef WIN32
#include <ws2tcpip.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
typedef unsigned int in_addr_t;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#endif

#include <sstream>
#include <vector>

enum ServerResult {
    OK = 0,

    ConnectionError = 1,
    WrongPassword,
    LoginFailure,
    ConnectionTimeout,
    ServerError,
    AlreadyLoggedOff,
    NotAllClientsConnected,
};

/**
 * Client information, as returned by the server
 */
struct CClientInfo {
    uint64_t id;
    std::string name;
    bool ready = false;
};

struct CServerInfo {
    std::string name;
    size_t max_clients;
    std::vector<CClientInfo> clients;
};

/// i do not want to include curlpp here (but maybe I should?)
namespace curlpp
{
class Easy;
}

namespace familyline::net
{
/**
 * Client-side server communication routines
 *
 * No, this is not the hungarian notation way of saying 'Server class'.
 */
class CServer
{
public:
    CServer() {}

    /**
     * Log into the server.
     *
     * Returns a certain result.
     */
    ServerResult login(std::string address, std::string username);

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
    ServerResult logout();

    ServerResult getServerInfo(CServerInfo& info);
    ServerResult toggleReady(bool);
    ServerResult connect();
    
    uint64_t getUserID() const;
    std::string getAddress() const;
    
    bool isReady() const;
    bool isLogged() const;
    bool isConnecting() const;
    
private:
    /// The address used to communicate with the HTTP part of the game protocol
    std::string http_address_;

    /// The client token, used to communicate with the server
    ///
    /// Obtained on login.
    std::string client_token_ = "";

    /// Is the client ready? (according to the server.)
    bool isReady_ = false;
    
    /// This client user ID.
    uint64_t userID_;

    /// The address and port used to communicate with the game
    std::string address_ = "";
    int port_ = 0;

    /// Timeout for each request
    int timeout_secs_ = 10;

    ServerResult checkErrors(unsigned httpcode, std::stringstream& body);

    /**
     * Build a basic curlpp request.
     *
     * Since most request parameters will be similar, we can do this
     *
     * We also request a string stream as data because we will use it, and we cannot
     * create the stream in the stack because curlpp will use this object on the perform()
     * method, who runs after this function returns.
     *
     * Returns a stringstream where the response body will be stored.
     */
    std::stringstream buildRequest(
        curlpp::Easy& req, std::string endpoint, std::string method = "GET", bool jsonbody = false,
        std::string data = "");
};

}  // namespace familyline::net
