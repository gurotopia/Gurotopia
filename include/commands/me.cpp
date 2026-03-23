#include "pch.hpp"
#include "me.hpp"

void me(ENetEvent& event, const std::string_view text)
{
        if (text.length() <= sizeof("me ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /me `w{message}``" });
        return;
    }
    std::string message{ text.substr(sizeof("me ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&pPeer, message](ENetPeer& peer)
    {
        packet::create(peer, false, 0, {
            "OnTalkBubble",
            pPeer->netid,
            std::format("player_chat= `6<```{}{}`` `#{}```6>``", 
                pPeer->prefix, pPeer->ltoken[0], message).c_str(),
            0u
        });
        packet::create(peer, false, 0, {
            "OnConsoleMessage",
            std::format("CP:0_PL:0_OID:__CT:[W]_ `6<```{}{}`` `#{}```6>``", 
                pPeer->prefix, pPeer->ltoken[0], message).c_str()
        });
    });
}
