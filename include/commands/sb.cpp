#include "pch.hpp"
#include "network/packet.hpp"
#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    std::string message{ text.substr(sizeof("sb ")-1) };
    auto &peer = _peer[event.peer];
    short total = peers(event).size();
    
    if (peer->gems < total * 40) 
    {
        gt_packet(*event.peer, false, 0, {
            "OnConsoleMessage",
            std::format("you need `${} Gems`` to Super Broadcast!", total * 40).c_str() // @todo get rgt message
        });
        return;
    }

    peers(event, PEER_ALL, [&](ENetPeer& p) 
    {
        gt_packet(p, false, 0, {
            "OnConsoleMessage",
            std::format(
                "CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{}{}`````5) in [```${}```5] ** : ```${}``",
                peer->prefix, peer->ltoken[0], peer->recent_worlds.back(), message
            ).c_str()
        });
    });
}