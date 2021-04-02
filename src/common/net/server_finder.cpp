
#include <config.h>
#include <fcntl.h>

#include <common/logger.hpp>
#include <common/net/net_common.hpp>
#include <common/net/server_finder.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <thread>

#ifdef WIN32
#define errno  WSAGetLastError()
#endif

using namespace familyline::net;
using json = nlohmann::json;

std::string getPrimaryAddress()
{
#ifdef FLINE_NET_SUPPORT

#ifdef __linux__
    struct ifaddrs* ifs;
    getifaddrs(&ifs);
    struct ifaddrs* base_ifs = ifs;

    if (ifs) {
        struct ifaddrs* baseif = ifs;

        for (; ifs; ifs = ifs->ifa_next) {
            if (ifs->ifa_flags & IFF_LOOPBACK || !(ifs->ifa_flags & IFF_MULTICAST)) {
                continue;
            }

            if (!ifs->ifa_addr) continue;

            char ip[128] = {};
            switch (ifs->ifa_addr->sa_family) {
                case AF_INET:
                    inet_ntop(
                        ifs->ifa_addr->sa_family, &((struct sockaddr_in*)ifs->ifa_addr)->sin_addr,
                        ip, sizeof(struct sockaddr));
                    break;
                default: continue;
            }

            printf("%s: %s\n", ifs->ifa_name, ip);
            auto sip = std::string{ip};
            freeifaddrs(base_ifs);
            return sip;
        }

        freeifaddrs(base_ifs);
    }
#endif

#if defined(HAS_UTSNAME) && !defined(WIN32)
    struct utsname buf;
    if (uname(&buf) == 0) {
        if (strlen(buf.nodename) > 0) {
            return std::string{buf.nodename};
        }
    }
#endif
#endif

    return std::string{"localhost"};
}

/**
 * Gets the local IP
 *
 * To get the current local IP, we need to get the machine hostname, and resolve it,
 * because this will usually get a valid IP for us
 *
 * We could get the IP by querying each interface, but this would not be much portable
 */
in_addr ServerFinder::getLocalIP()
{
#ifdef FLINE_NET_SUPPORT
    struct addrinfo* addrdata;
    auto hostname = getPrimaryAddress();
    auto& log     = LoggerService::getLogger();
    struct in_addr local_addr;

    if (getaddrinfo(hostname.c_str(), NULL, NULL, &addrdata)) {
        log->write(
            "server-finder", LogType::Error,
            "could not resolve IP for hostname %s (%x/%s), defaulting to loopback",
            hostname.c_str(), errno, gai_strerror(errno));

        local_addr.s_addr = inet_addr("127.0.0.1");
        return local_addr;
    }

    if (!addrdata) {
        log->write(
            "server-finder", LogType::Error, "no IP address found for hostname %s",
            hostname.c_str());

        local_addr.s_addr = inet_addr("127.0.0.1");
        return local_addr;
    }

    struct addrinfo* baseaddrdata = addrdata;
    while (addrdata) {
        // skip ipv6 for now.
        if (addrdata->ai_family != AF_INET) {
            addrdata = addrdata->ai_next;
            continue;
        }

        local_addr = ((struct sockaddr_in*)addrdata->ai_addr)->sin_addr;
        addrdata   = addrdata->ai_next;
    }

    freeaddrinfo(baseaddrdata);

    return local_addr;

#else
    throw net_exception("Network support is not compiled in!");

#endif
}

ServerFinder::ServerFinder()
{
    auto& log = LoggerService::getLogger();
    
#ifdef FLINE_NET_SUPPORT

    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        throw net_exception(fmt::format(
            "Could not create socket for server finder: {} ({})", strerror(errno), errno));
    }

    mcast_addr_            = {};
    mcast_addr_.sin_family = AF_INET;
    mcast_addr_.sin_port   = htons(multicast_port_);
    inet_pton(AF_INET, multicast_ip_str_, &mcast_addr_.sin_addr.s_addr);

    auto local_addr = getLocalIP();
    char buf[61]    = {};
    log->write(
        "server-finder", LogType::Info, "IP address is %s",
        inet_ntop(AF_INET, &local_addr, buf, 60));

    if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_IF, (char*)&local_addr, sizeof(local_addr)) <
        0) {
        throw net_exception(fmt::format(
            "Could not set multicast interface for server finder: {} ({})", strerror(errno),
            errno));
    }

#else
    log->write(
        "server-finder", LogType::Error, "Network support is not compiled in!");

#endif
}

ServerFinder::~ServerFinder()
{
    if (discovering_) this->stopDiscover();

    close(socket_);
}

std::vector<std::string> splitLines(std::string_view s, char sep = '\n')
{
    auto strptr = s;
    std::vector<std::string> r;

    auto lastidx = 0;
    auto idx     = 0;

    do {
        lastidx = idx;
        idx     = s.find_first_of(sep, lastidx);
        strptr  = s.substr(lastidx, idx - lastidx);

        std::string s{strptr};

        if (auto cr = s.find_first_of('\r'); cr != std::string::npos) s.erase(cr);

        r.push_back(s);

        if (idx != std::string_view::npos) idx += 1;

    } while (idx != std::string_view::npos);

    return r;
}

/**
 * Parse the HTTP-based information of the discovery response
 *
 * Return the server information, or nullopt if the server information is
 * invalid
 */
std::optional<ServerInfo> parseDiscoverInformation(std::string_view data)
{
#ifdef FLINE_NET_SUPPORT

    auto lines = splitLines(data);

    bool http_message   = false;
    bool content_starts = false;

    std::optional<ServerInfo> si = {};

    std::string addr;
    unsigned int port = 8100;

    std::string json_data = {};
    std::string version;

    for (auto& l : lines) {
        if (l.starts_with("HTTP") && l.ends_with("200 OK")) {
            http_message   = true;
            content_starts = false;
            si             = {};
        }

        if (l.starts_with("Location")) {
            if (auto midx = l.find_first_of(' '); midx != std::string::npos) {
                auto lval = l.substr(midx + 1);

                // Location has the format
                //    http://<url>
                //    url can be an IP or a hostname, followed by an optional port value after a
                //    colon

                if (auto httpos = lval.find("http"); httpos != std::string::npos) {
                    auto residx  = httpos + 7;
                    auto portidx = lval.find_first_of(':', residx);

                    if (portidx != std::string::npos) {
                        addr = lval.substr(residx, portidx - residx);

                        port = std::atoi(lval.substr(portidx + 1).c_str());
                    } else {
                        addr = lval.substr(residx + 1);
                    }
                }
            }
        }

        if (l == "" && http_message && !content_starts) {
            content_starts = true;
            continue;
        }

        if (l.starts_with("Server")) {
            if (auto midx = l.find_first_of(' '); midx != std::string::npos) {
                auto lval = l.substr(midx + 1);

                if (lval.starts_with("familyline-server")) {
                    int versionval = strlen("familyline-server") + 1;

                    version = lval.substr(versionval);
                } else {
                    return std::nullopt;
                }
            }
        }

        if (content_starts) {
            json_data      = l;
            content_starts = false;
            http_message   = false;
        }
    }

    if (json_data.size() == 0) {
        return std::nullopt;
    }

    auto body = json::parse(json_data);

    ServerInfo sinfo   = {};
    sinfo.ip_addr      = addr;
    sinfo.port         = port;
    sinfo.version      = version;
    sinfo.name         = body["name"];
    sinfo.description  = "";
    sinfo.player_count = body["num_clients"];
    sinfo.player_max   = body["max_clients"];

    return std::make_optional(sinfo);

#else
    throw net_exception("Network support is not compiled in!");
#endif
};

/// Start the server discovery process.
///
/// You will need to register a callback; the server finder will call
/// it for each server it discovers
///
/// The discovery will happen in a separate thread, so watch out.
void ServerFinder::startDiscover(discovery_cb callback)
{
#ifdef FLINE_NET_SUPPORT

    discovering_  = true;
    thr_discover_ = std::thread(
        [](SOCKET& s, struct sockaddr_in multicastaddr, bool& operating, discovery_cb cb) {
            int seconds_between_discover = 5;
            auto& log                    = LoggerService::getLogger();
            log->write("server-finder", LogType::Info, "starting discover");

            std::unordered_map<std::string, ServerInfo> servers;

            auto send_discover_message = [&]() {
                std::string message =
                    "M-SEARCH * \r\n"
                    "MAN: \"ssdp:discover\"\r\n"
                    "ST: game_server:familyline1\r\n"
                    "\r\n";

                if (sendto(
                        s, message.data(), message.size(), 0, (struct sockaddr*)&multicastaddr,
                        sizeof(multicastaddr)) < 0) {
                    log->write(
                        "server-finder", LogType::Error,
                        "sendto() failed when sending discovery message: %d (%s)", errno,
                        strerror(errno));
                    return;
                }
            };

            send_discover_message();
            auto last_discover = time(nullptr);
            usleep(2000);

// Use a non-blocking receive so it does not block the rest of the game.
//
// We need to be able to detect that the discovery has been canceled.
#ifdef WIN32
            u_long iMode = 1;
            ioctlsocket(s, FIONBIO, &iMode);
#endif

            while (operating) {
                auto discover = time(nullptr);
                if (discover - last_discover >= seconds_between_discover) {
                    send_discover_message();
                    last_discover = discover;
                }

                char buf[4096] = {};
                if (recvfrom(s, buf, 4095, MSG_DONTWAIT, nullptr, 0) < 0) {
                    switch (errno) {
                        case EWOULDBLOCK: break;
                        default:
                            log->write(
                                "server-finder", LogType::Error,
                                "recvfrom() failed when receiving response of discovery message: "
                                "%d (%s)",
                                errno, strerror(errno));
                            return;
                    }

                    continue;
                }

                log->write("server-finder", LogType::Info, "received something");

                auto data = parseDiscoverInformation(std::string_view{buf, 4096});
                if (data && !servers.contains(data->ip_addr)) {
                    cb(*data);
                    servers[data->ip_addr] = *data;
                }
            }

            puts(":(");
        },
        std::ref(socket_), mcast_addr_, std::ref(discovering_), callback);

#else
    throw net_exception("Network support is not compiled in!");

#endif
}

/// Stop the server discovery process
void ServerFinder::stopDiscover()
{
    discovering_ = false;
    auto& log    = LoggerService::getLogger();
    thr_discover_.join();
    log->write("server-finder", LogType::Info, "stopping discover");
}
