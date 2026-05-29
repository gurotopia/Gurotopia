#include "pch.hpp"
#include "tools/string.hpp"

#include "who.hpp"

void who(ENetEvent& event, const std::string_view text) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> names;
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&pPeer, event, &names](ENetPeer& peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer.data);

        std::string full_name = std::format("`{}{}", pOthers->prefix, pOthers->ltoken[0]);
        if (pOthers->user_id != pPeer->user_id)
        {
            packet::create(*event.peer, false, 0, { "OnTalkBubble", pOthers->netid, full_name.c_str(), 1u });
        }
        names.emplace_back(std::move(full_name));
    });
    packet::action(*event.peer, "log", std::format(
        "msg|`wWho's in `${}``:`` {}``",
        pPeer->recent_worlds.back(), join(names, ", ")
    ));
}
