#include "pch.hpp"
#include "who.hpp"

void who(ENetEvent& event, const std::string_view text) 
{
    auto &peer = _peer[event.peer];
    std::string list;

    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, event, &list](ENetPeer& p)
    {
        auto &w_peer = _peer[&p]; // @note 'w' is just short for world. so world_peer
        std::string full_name = std::format("`{}{}", w_peer->prefix, w_peer->ltoken[0]);
        if (w_peer->user_id != peer->user_id)
        {
            packet::create(*event.peer, false, 0, {
                "OnTalkBubble",
                w_peer->netid,
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
