#include "pch.hpp"
#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    std::string message{ text.substr(sizeof("sb ")-1) };
    auto &peer = _peer[event.peer];

    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return;

    std::string display = peer->recent_worlds.back();
    for (block &b : it->second.blocks)
        if (b.fg == 226 && b.toggled) 
        {
            display = "`4JAMMED``";
            break; // @note we don't care if other signals are toggled.
        }

    peers(event, PEER_ALL, [&peer, message, display](ENetPeer& p) 
    {
        gt_packet(p, false, 0, {
            "OnConsoleMessage",
            std::format(
                "CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{}{}`````5) in [```${}```5] ** : ```${}``",
                peer->prefix, peer->ltoken[0], display, message
            ).c_str()
        });
    });
}