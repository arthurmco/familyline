#pragma once

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

#ifdef __linux__
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#ifdef WIN32
#define MSG_DONTWAIT 0
#define usleep(x) Sleep(x/1000)
#define close(x) closesocket(x)
#define errno  WSAGetLastError()
#endif

#include <fmt/format.h>
#include <string>
#include <string_view>
#include <stdexcept>

#define HAS_UTSNAME __has_include(<sys/utsname.h)

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
