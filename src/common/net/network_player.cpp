#include <common/logger.hpp>
#include <common/net/net_common.hpp>
#include <common/net/network_player.hpp>
#include <variant>

using namespace familyline::net;
using namespace familyline::logic;

void NetworkPlayer::generateInput()
{
    auto& log = LoggerService::getLogger();

    for (net::Packet packet; client_.peek(packet);) {
        if (Packet::InputRequest* req = std::get_if<Packet::InputRequest>(&packet.message); req) {
            log->write(
                "network-player", LogType::Debug, "received packet, from=%zu, tick=%zu",
                req->client_from, packet.tick);
            this->pushAction(req->input, packet.tick);
        }
        
        client_.pop();                    
    }
}
