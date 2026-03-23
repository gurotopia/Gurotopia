#include "pch.hpp"
#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("sb ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /sb `w{message}``" });
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

    peers("", PEER_ALL, [&pPeer, message, display](ENetPeer& peer) 
    {
        packet::create(peer, false, 0, {
            "OnConsoleMessage",
            std::format(
                "CP:0_PL:0_OID:_CT:[SB]_ `5** from (`{}{}`````5) in [```${}```5] ** : ```${}``",
                pPeer->prefix, pPeer->ltoken[0], display, message
            ).c_str()
        });
    });
}