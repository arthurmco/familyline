#pragma once

/**
 * This class will help trying to find servers in your LAN.
 *
 */

#include <fmt/format.h>

#include <cstdarg>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

namespace familyline::net
{
class net_exception : public std::runtime_error
{
protected:
    std::string_view _message;
    std::string msg_;

public:
    explicit net_exception(std::string_view message) : std::runtime_error(message.data())
    {
        msg_ = fmt::format("Network error {}", message.data());
    }

    virtual const char* what() const noexcept { return msg_.c_str(); }
};

}  // namespace familyline::net

#ifndef WIN32
#define SOCKET int
#endif

#ifdef WIN32
#define _WINSOCKAPI_
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <io.h>
#define close _close

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

namespace familyline::net
{
struct ServerInfo {
    std::string ip_addr;
    unsigned int port;
    std::string name;
    std::string version;
    std::string description;
    size_t player_count;
    size_t player_max;
};

using discovery_cb = std::function<void(ServerInfo)>;

class ServerFinder
{
public:
    ServerFinder();
    ~ServerFinder();

    /// Start the server discovery process.
    ///
    /// You will need to register a callback; the server finder will call
    /// it for each server it discovers
    ///
    /// The discovery will happen in a separate thread, so watch out.
    void startDiscover(discovery_cb callback);

    /// Stop the server discovery process
    void stopDiscover();

private:
    bool discovering_ = false;

    SOCKET socket_;

    std::thread thr_discover_;
    struct sockaddr_in mcast_addr_ = {};

    const char* multicast_ip_str_ = "239.255.255.250";
    const int multicast_port_     = 1983;

    std::unordered_map<std::string /*ip*/, ServerInfo> servers_;

    /**
     * Gets the local IP
     *
     * To get the current local IP, we need to get the machine hostname, and resolve it,
     * because this will usually get a valid IP for us
     *
     * We could get the IP by querying each interface, but this would not be much portable
     */
    in_addr getLocalIP();
};
}  // namespace familyline::net
