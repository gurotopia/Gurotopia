#include "pch.hpp"
#include "network/packet.hpp"
#include "me.hpp"

void me(ENetEvent& event, const std::string_view text)
{
    std::string message{ text.substr(sizeof("me ")-1) };
    auto &peer = _peer[event.peer];

    peers(event, PEER_SAME_WORLD, [&peer, message](ENetPeer& p)
    {
        gt_packet(p, false, 0, {
            "OnTalkBubble",
            peer->netid,
            std::format("player_chat= `6<```{}{}`` `#{}```6>``", 
                peer->prefix, peer->ltoken[0], message).c_str(),
            0u
        });
        gt_packet(p, false, 0, {
            "OnConsoleMessage",
            std::format("CP:0_PL:0_OID:__CT:[W]_ `6<```{}{}`` `#{}```6>``", 
                peer->prefix, peer->ltoken[0], message).c_str()
        });
    });
}
