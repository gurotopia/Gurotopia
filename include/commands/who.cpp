#include "pch.hpp"
#include "who.hpp"

void who(ENetEvent& event, const std::string_view text) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    std::string list;

    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, event, &list](ENetPeer& p)
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        std::string full_name = std::format("`{}{}", _p->prefix, _p->ltoken[0]);
        if (_p->user_id != peer->user_id)
        {
            packet::create(*event.peer, false, 0, {
                "OnTalkBubble",
                _p->netid,
                full_name.c_str(),
                1u
            });
        }
        if (!list.empty()) list += ", ";
        list.append(std::move(full_name));
    });
    packet::action(*event.peer, "log", std::format(
        "msg|`wWho's in `${}``:`` {}``",
        peer->recent_worlds.back(), list
    ));
}
