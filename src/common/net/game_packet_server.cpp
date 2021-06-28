
#include <config.h>
#include <flatbuffers/flatbuffers.h>
#include <fmt/format.h>

#include "common/logic/input_recorder.hpp"

#ifdef __linux__
#include <bits/stdint-uintn.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <chrono>
#include <common/logger.hpp>
#include <common/logic/input_reproducer.hpp>
#include <common/net/game_packet_server.hpp>
#include <common/net/server.hpp>
#include <iterator>
#include <mutex>
#include <optional>
#include <thread>
#include <tl/expected.hpp>
#include <variant>

#include "network_generated.h"

#ifdef WIN32
#define errno WSAGetLastError()
#endif

using namespace familyline::net;

Packet toNativePacket(const ::familyline::NetPacket* p)
{
    auto& log = familyline::LoggerService::getLogger();

    decltype(Packet::message) message = std::monostate{};
    switch (p->message_type()) {
        case familyline::Message_sreq: {
            auto m  = p->message_as_sreq();
            message = Packet::NStartRequest{m->client_id(), m->token()->str()};
            break;
        }
        case familyline::Message_sres: {
            auto m  = p->message_as_sres();
            message = Packet::NStartResponse{m->client_ack(), m->all_clients_ack()};
            break;
        }
        case familyline::Message_lreq: {
            auto m  = p->message_as_lreq();
            message = Packet::NLoadingRequest{m->percent()};
            break;
        }
        case familyline::Message_lres: {
            auto m  = p->message_as_lres();
            message = Packet::NLoadingResponse{m->percent()};
            break;
        }
        case familyline::Message_greq: {
            auto m  = p->message_as_greq();
            message = Packet::NGameStartRequest{m->reserved()};
            break;
        }
        case familyline::Message_gres: {
            auto m  = p->message_as_gres();
            message = Packet::NGameStartResponse{m->reserved()};
            break;
        }
        case familyline::Message_ireq: {
            auto m = p->message_as_ireq();

            auto itype = familyline::logic::deserializeInputAction(
                m, [](const familyline::InputRequest* r) { return r->input_msg_type(); },
                [&](const familyline::InputRequest* r, familyline::InputType type) {
                    switch (type) {
                        case familyline::InputType_cmd: return (void*)r->input_msg_as_cmd();
                        case familyline::InputType_sel: return (void*)r->input_msg_as_sel();
                        case familyline::InputType_obj_move:
                            return (void*)r->input_msg_as_obj_move();
                        case familyline::InputType_cam_move:
                            return (void*)r->input_msg_as_cam_move();
                        case familyline::InputType_cam_rotate:
                            return (void*)r->input_msg_as_cam_rotate();
                        case familyline::InputType_create: return (void*)r->input_msg_as_create();
                        default:
                            log->write(
                                "game-packet-server", familyline::LogType::Fatal,
                                "Read input message type (%02x)", type);
                            return (void*)nullptr;
                    }
                });
            message = Packet::InputRequest{m->client_from(), itype};
            printf("aaa\n");
            break;
        }
        case familyline::Message_ires: {
            auto m  = p->message_as_ires();
            message = Packet::InputResponse{m->client_from(), m->client_ack()};
            break;
        }
        case familyline::Message_NONE: {
            auto m  = p->message_as_ires();
            message = std::monostate{};  // Packet::NGameStartResponse{m->reserved()};
            break;
        }
    }

    std::chrono::seconds d{p->timestamp()};
    return Packet{p->tick(), p->source_client(), p->dest_client(), d, p->id(), message};
}

/// This will allow us to use std::visit with multiple variants at once, a thing
/// that should be part of C++20.
/// There are FOUR locations that I use this. I hope this gets suggested for C++23
/// or C++26
template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

flatbuffers::Offset<::familyline::NetPacket> GamePacketServer::toSerializedPacket(
    const Packet& p, flatbuffers::FlatBufferBuilder& b)
{
    familyline::Message msg_type = familyline::Message_NONE;
    flatbuffers::Offset<void> msg_data;

    std::visit(
        overload{
            [&](const std::monostate& m) {
                auto& log = LoggerService::getLogger();

                log->write(
                    "game-packet-server", LogType::Error,
                    "to be sent message (timestamp %ull, id %d) is invalid (has invalid type)",
                    p.timestamp.count(), p.id);
                msg_type = familyline::Message_NONE;
            },
            [&](const Packet::NStartRequest& a) {
                auto token = b.CreateString(a.token);
                auto cval  = familyline::CreateStartRequest(b, a.client_id, token);

                msg_type = familyline::Message_sreq;
                msg_data = cval.Union();
            },
            [&](const Packet::NStartResponse& m) {
                auto cval = familyline::CreateStartResponse(b, m.client_ack, m.all_clients_ack);

                msg_type = familyline::Message_sres;
                msg_data = cval.Union();
            },
            [&](const Packet::NLoadingRequest& m) {
                auto cval = familyline::CreateLoadingRequest(b, m.percent);

                msg_type = familyline::Message_lreq;
                msg_data = cval.Union();
            },
            [&](const Packet::NLoadingResponse& m) {
                auto cval = familyline::CreateLoadingResponse(b, m.percent);

                msg_type = familyline::Message_lres;
                msg_data = cval.Union();
            },
            [&](const Packet::NGameStartRequest& m) {
                auto cval = familyline::CreateGameStartRequest(b, m.val);

                msg_type = familyline::Message_greq;
                msg_data = cval.Union();
            },
            [&](const Packet::NGameStartResponse& m) {
                auto cval = familyline::CreateGameStartResponse(b, m.val);

                msg_type = familyline::Message_gres;
                msg_data = cval.Union();
            },
            [&](const Packet::InputRequest& m) {
                familyline::InputType type_val;
                flatbuffers::Offset<void> type_data;
                familyline::logic::serializeInputAction(m.input, type_val, type_data, b);

                auto cval = familyline::CreateInputRequest(b, m.client_from, type_val, type_data);

                msg_type = familyline::Message_ireq;
                msg_data = cval.Union();
            },
            [&](const Packet::InputResponse& m) {
                auto cval = familyline::CreateInputResponse(b, m.client_from, m.client_ack);

                msg_type = familyline::Message_ires;
                msg_data = cval.Union();
            }

        },
        p.message);

    auto fbpacket = familyline::CreateNetPacket(
        b, p.tick, p.source_client, p.dest_client, p.timestamp.count(), p.id, msg_type, msg_data);
    return fbpacket;
}

bool GamePacketServer::connect()
{
    auto& log = LoggerService::getLogger();

#ifdef FLINE_NET_SUPPORT

    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (socket_ < 0) {
        throw net_exception(fmt::format(
            "Could not create socket for game packet server: {} ({})", strerror(errno), errno));
    }

    sockaddr_in addr = {};
    inet_pton(AF_INET, address_.c_str(), &addr.sin_addr.s_addr);
    addr.sin_port   = htons(port_);
    addr.sin_family = AF_INET;

    if (::connect(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log->write(
            "game-packet-server", LogType::Error,
            "Failed to connect to game packet server at %s:%d (%s)", address_.c_str(), port_,
            strerror(errno));

        return false;
    }

    auto epoch_secs =
        duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    send_queue_.push(Packet{
        0, (unsigned long long)id_, 0, epoch_secs, 1,
        Packet::NStartRequest{(unsigned long long)id_, token_}});

    connected_ = true;
    return true;

#else

    log->write(
        "game-packet-server", LogType::Fatal,
        "Cannot connect because you did not compile the network support!");
    log->write(
        "game-packet-server", LogType::Fatal,
        "Please run cmake again with -DFILE_NET_SUPPORT:BOOL=on");

    throw net_exception("Network support is not compiled in!");
    return false;

#endif
}

/**
 * Send pending messages to their destinations and receive messages
 * from the clients
 */
void GamePacketServer::update()
{
#ifdef FLINE_NET_SUPPORT
    if (!connected_) return;

    auto& log = LoggerService::getLogger();

    std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    while (!send_queue_.empty()) {
        send_mutex_.lock();
        auto packet = send_queue_.front();
        packet.id   = ++last_message_id_;
        log->write(
            "game-packet-server", LogType::Info,
            "sending package (id %llu, from %llu, to %llu, tick %llu, timestamp %llu..., type %d )", packet.id,
            packet.source_client, packet.dest_client, packet.tick, packet.timestamp, packet.message.index());

        auto byte_data = this->createMessage(packet);

        send_queue_.pop();
        send_mutex_.unlock();

        auto r = send(socket_, byte_data.data(), byte_data.size(), 0);

        if (r < 0) {
            log->write(
                "game-packet-server", LogType::Error, "Error while sending a packet: %d (%s)",
                errno, strerror(errno));
        }

        if (r != byte_data.size()) {
            log->write(
                "game-packet-server", LogType::Error,
                "Data send was different than packet size (%d != %zu)", r, byte_data.size());
        }

        log->write("game-packet-server", LogType::Info, "data sent! (%zu)", byte_data.size());
    }

    uint8_t data[1024] = {};
    auto r             = recv(socket_, data, 1023, MSG_DONTWAIT);
    if (r == 0) {
        log->write("game-packet-server", LogType::Error, "Connection terminated");
        connected_ = false;
        return;
    }

    if (r < 0) {
        if (errno == EWOULDBLOCK) {
            // no message to receive
            return;
        }

        log->write(
            "game-packet-server", LogType::Error, "Error while receiving a packet: %d (%s)", errno,
            strerror(errno));
        return;
    }

    std::vector<uint8_t> vdata(data, data + r);
    auto pkts = this->decodeMessage(vdata);
    if (pkts.size() == 0) {
        log->write(
            "game-packet-server", LogType::Error,
            "received invalid packet (len %zu, starts with %02x %02x %02x %02x", r,
            r > 0 ? vdata[0] : 0, r > 1 ? vdata[1] : 0, r > 2 ? vdata[2] : 0, r > 3 ? vdata[3] : 0);
    }

    for (auto& pkt : pkts) {
        log->write(
            "game-packet-server", LogType::Info,
            "received packet (id %llu, from %llu, to %llu, tick %llu, timestamp %llu..., type %d )", pkt.id,
            pkt.source_client, pkt.dest_client, pkt.tick, pkt.timestamp, pkt.message.index());
        receive_mutex_.lock();
        if (dispatch_client_messages_ && pkt.source_client != 0) {
            client_receive_queue_[pkt.source_client].push(pkt);
        } else {
            receive_queue_.push(pkt);
        }
        receive_mutex_.unlock();
    }

#else
    throw net_exception("Network support is not compiled in!");

#endif
}

/**
 * Enqueue a packet
 */
void GamePacketServer::enqueuePacket(Packet&& p)
{
    std::lock_guard<std::mutex> lg(this->send_mutex_);
    send_queue_.push(p);
}

/**
 * Poll a packet
 *
 * Returns true if we have a packet, false if we do not
 */
bool GamePacketServer::pollPacketFor(uint64_t id, Packet& p)
{
#ifdef FLINE_NET_SUPPORT
    if (!client_receive_queue_.contains(id)) return false;

    if (client_receive_queue_[id].empty()) return false;

    p = client_receive_queue_[id].front();
    client_receive_queue_[id].pop();

    return true;
#else
    throw net_exception("Network support is not compiled in!");

#endif
}

/**
 * Wait for all clients to be connected, or until a timeout (in seconds)
 *
 * This future will only be valid after all clients are connected.
 * If no clients are connected, it will return a NetResult, matching the ServerError we found.
 * Not all server errors will be returned, though
 */
std::future<tl::expected<std::vector<NetworkClient>, NetResult>>
GamePacketServer::waitForClientConnection(int timeout)
{
#ifdef FLINE_NET_SUPPORT

    using RetT = std::vector<NetworkClient>;
    using RetE = NetResult;
    return std::async(std::launch::async, [this, timeout]() -> tl::expected<RetT, RetE> {
        bool all_ack = false;
        auto start   = std::chrono::system_clock::now();
        std::vector<uint64_t> player_ids;

        while (!all_ack) {
            // check if we timed out
            auto current = std::chrono::system_clock::now();
            if (current - start > std::chrono::seconds(timeout)) {
                return tl::make_unexpected(NetResult::ConnectionTimeout);
            }

            if (!this->connected_) {
                return tl::make_unexpected(NetResult::UnexpectedDisconnect);
            }

            auto& log = LoggerService::getLogger();

            while (!this->receive_queue_.empty()) {
                std::lock_guard<std::mutex> lg(this->receive_mutex_);
                auto& pkt = receive_queue_.front();

                printf("received a message!");
                if (auto p = std::get_if<Packet::NStartResponse>(&pkt.message); p) {
                    log->write(
                        "game-packet-server", LogType::Info, "client id %llx ack'ed",
                        p->client_ack);

                    // Do not add ourselves to the client list
                    // The clients represents the other people.
                    if (p->client_ack != id_) player_ids.push_back(p->client_ack);

                    if (p->all_clients_ack) {
                        all_ack = true;
                    }
                }

                receive_queue_.pop();

                if (all_ack) {
                    log->write("game-packet-server", LogType::Info, "all clients ack'ed");
                    break;
                }
            }
        }

        std::vector<NetworkClient> clients;
        dispatch_client_messages_ = true;
        std::transform(
            player_ids.begin(), player_ids.end(), std::back_inserter(clients),
            [this](uint64_t& id) {
                using namespace std::placeholders;
                return NetworkClient(
                    id, std::bind(&GamePacketServer::enqueuePacket, this, _1),
                    std::bind(&GamePacketServer::pollPacketFor, this, id, _1));
            });

        return tl::expected<RetT, RetE>(std::move(clients));
    });
#else
    throw net_exception("Network support is not compiled in!");
#endif
}

/**
 * A way to do multiple push_back()s in one line
 *
 * ```
 * push_multi(v, 1, 2, 3);
 * ```
 *
 * will be the same as if you did
 *
 * ```
 * v.push_back(1);
 * v.push_back(2);
 * v.push_back(3);
 * ```
 */
void push_multi(std::vector<uint8_t>& v) {}

template <typename... Ts>
void push_multi(std::vector<uint8_t>& v, uint8_t val, Ts... args)
{
    v.push_back(val);
    push_multi(v, args...);
}

uint32_t calculateCRC(const std::vector<uint8_t>& data) { return 0x12345678; }

uint32_t getU32(const std::vector<uint8_t>& data, int offset)
{
    return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) |
           (data[offset + 3] << 24);
}

bool validateCRC(const std::vector<uint8_t>& data, uint32_t expected)
{
    uint32_t checksum = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
    return checksum == expected;
}

std::vector<uint8_t> GamePacketServer::createMessage(const Packet& p)
{
#ifdef FLINE_NET_SUPPORT
    flatbuffers::FlatBufferBuilder fbb;
    auto packet = toSerializedPacket(p, fbb);
    FinishNetPacketBuffer(fbb, packet);

    std::string magic = "FAMI";
    uint32_t size     = fbb.GetSize();
    std::vector<uint8_t> ret;
    std::vector<uint8_t> data(fbb.GetBufferPointer(), fbb.GetBufferPointer() + size);

    ret.insert(ret.begin(), magic.begin(), magic.begin() + 4);
    push_multi(ret, 0, 0, 0, 0);  // flags
    push_multi(ret, 0, 0, 0, 0);  // checksum (will be fixed later)
    push_multi(ret, size & 0xff, (size >> 8) & 0xff, (size >> 16) & 0xff, (size >> 24) & 0xff);

    std::copy(data.begin(), data.end(), std::back_inserter(ret));

    uint32_t checksum = calculateCRC(ret);
    ret[8]            = checksum & 0xff;
    ret[9]            = (checksum >> 8) & 0xff;
    ret[10]           = (checksum >> 16) & 0xff;
    ret[11]           = (checksum >> 24) & 0xff;

    return ret;
}

std::vector<Packet> GamePacketServer::decodeMessage(std::vector<uint8_t> data)
{
    auto& log = LoggerService::getLogger();

    if (data.size() <= 16) {
        log->write(
            "game-packet-server", LogType::Error,
            "received a packet too small (%zu bytes, minimum is 16+1)", data.size());
        return std::vector<Packet>();
    }

    if (data[0] != 'F' || data[1] != 'A' || data[2] != 'M' || data[3] != 'I') {
        log->write(
            "game-packet-server", LogType::Error,
            "received a packet with an incorrect header (%02x %02X %02x %02x)", data[0], data[1],
            data[2], data[3]);
        return std::vector<Packet>();
    }

    uint32_t flags = getU32(data, 4);
    if (flags != 0) {
        return std::vector<Packet>();
    }

    uint32_t size = getU32(data, 12);

    std::vector<Packet> res;

    std::vector<uint8_t> packetdata;
    std::copy_n(data.begin(), size + 16, std::back_inserter(packetdata));

    auto pkt = GetNetPacket(packetdata.data() + 16);
    assert(packetdata[0] == 'F');

    if (pkt) {
        auto verifier = flatbuffers::Verifier{packetdata.data()+16, size};
        if (!VerifyNetPacketBuffer(verifier)) {
            log->write("game-packet-server", LogType::Error, "packet verifier failed!");
        } else {
            res.push_back(toNativePacket(pkt));
        }
    } else {
        log->write(
            "game-packet-server", LogType::Error, "flatbuffer conversion returned a null pointer!");
    }

    if (data.size() > packetdata.size()) {
        log->write(
            "game-packet-server", LogType::Warning,
            "extra data in this packet, maybe we received more than 1 packet in a message? (%zu vs "
            "%zu)",
            data.size(), packetdata.size());

        auto len = data.size() - packetdata.size();
        std::vector<uint8_t> newpacket;
        std::copy_n(data.begin() + packetdata.size(), len, std::back_inserter(newpacket));

        auto nres = decodeMessage(newpacket);
        std::copy(nres.begin(), nres.end(), std::back_inserter(res));
    }

    return res;

#else
    throw net_exception("Network support is not compiled in!");
#endif
}

Packet GamePacketServer::createPacket(
    uint64_t tick, uint64_t source, uint64_t dest, uint64_t id, decltype(Packet::message) message)
{
    auto timestamp =
        duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    return Packet{tick, source, dest, timestamp, id, message};
}

/**
 * Send an input message to the server (so it can tell other clients)
 */
void GamePacketServer::sendInputMessage(logic::PlayerInputAction& a)
{
    auto pkt = createPacket(
        a.tick, a.playercode, 0, a.timestamp, Packet::InputRequest{a.playercode, a.type});
    this->enqueuePacket(std::move(pkt));
}

/**
 * Send a message that will tell the server that you are loading data
 */
void GamePacketServer::sendLoadingMessage(int percent)
{
    auto pkt = createPacket(
        0, this->id_, 0, 1, Packet::NLoadingRequest{(unsigned short)(percent & 0xffff)});
    this->enqueuePacket(std::move(pkt));
}

/**
 * Send a message that will tell the server you are ready to start the game
 */
void GamePacketServer::sendStartMessage()
{
    auto pkt = createPacket(0, this->id_, 0, 2, Packet::NGameStartRequest{0});
    this->enqueuePacket(std::move(pkt));
}
