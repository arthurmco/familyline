#include <common/net/net_player_sender.hpp>

#include <common/logger.hpp>

using namespace familyline::net;
using namespace familyline::logic;

bool NetPlayerSender::listenPlayer(logic::PlayerInputAction a)
{
    if (a.playercode != human_id_)
        return false;
    
    auto& log = LoggerService::getLogger();
    static const char* action_names[] = {
        "CommandInput", "ObjectMove", "SelectAction", "AddSelectAction", "CreateSelectGroup",
        "CreateEntity", "SelectGroup", "AddSelectGroup", "RemoveSelectGroup",
        "CameraMove", "CameraRotate", "?", "?", "?"
    };
    
    log->write("net-player-sender", LogType::Debug, "sending packet to the server, tick=%zu, type: %s", a.tick,
               action_names[a.type.index()]);
    
    gps_.sendInputMessage(a);
    return true;
}
