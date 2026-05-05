#include "pch.hpp"
#include "on/ConsoleMessage.hpp"

#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("sb ") - 1) 
    {
        on::ConsoleMessage(event.peer, "Usage: /sb `w{message}``");
        return;
    }
    std::string message{ text.substr(sizeof("sb ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string display = pPeer->recent_worlds.back();
    for (::block &block : world->blocks)
        if (block.fg == 226 && block.state[2] & S_TOGGLE) 
        {
            display = "`4JAMMED``";
            break; // @note we don't care if other signals are toggled.
        }

    peers("", PEER_ALL, [&event, &pPeer, message, display](ENetPeer& peer) 
    {
        on::ConsoleMessage(event.peer, 
            std::format(
                "CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{}{}`````5) in [```${}```5] ** : ```${}``",
                pPeer->prefix, pPeer->growid, display, message
            )
        );
    });
}