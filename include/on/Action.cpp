#include "database/peer.hpp"
#include "network/packet.hpp"
#include "Action.hpp"

void Action(ENetEvent& event, const std::string_view text) 
{
    std::string_view to_slang = (text == "facepalm") ? "fp" : (text == "shrug") ? "idk" : (text == "foldarms") ? "fold" : (text == "fa") ? "fold" : (text == "stubborn") ? "fold" : text;
    std::string formatted_action{ "/" + std::string(to_slang) };
    // @todo does this need be sent to everyone?
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        gt_packet(p, true, 0, {
            "OnAction", 
            formatted_action.c_str()
        });
    });
}
