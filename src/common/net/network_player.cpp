#include <common/net/network_player.hpp>
#include "common/net/net_common.hpp"

#include <variant>

using namespace familyline::net;
using namespace familyline::logic;

void NetworkPlayer::generateInput()
{
    for (net::Packet packet; client_.peek(packet); ) {
        if (Packet::InputRequest* req = std::get_if<Packet::InputRequest>(&packet.message); req) {            
            this->pushAction(req->input, packet.tick);
        }
    }
}
