#include "pch.hpp"
#include "on/ConsoleMessage.hpp"

#include "me.hpp"

void me(ENetEvent& event, const std::string_view text)
{
    
    const std::string message{ text.substr(sizeof("me ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer, message](ENetPeer& peer)
    {
        send_varlist(&peer, {
            "OnTalkBubble",
            pPeer->netid,
            std::format(
                "player_chat= `6<```{}{}`` `#{}```6>``", 
                pPeer->prefix, pPeer->growid, message).c_str(),
            0u
        });
        on::ConsoleMessage(&peer, 
            std::format(
                "CP:0_PL:0_OID:__CT:[W]_ `6<```{}{}`` `#{}```6>``", 
                pPeer->prefix, pPeer->growid, message
            )
        );
    });
}
