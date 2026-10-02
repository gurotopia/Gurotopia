#include "pch.hpp"
#include "Action.hpp"

void on::Action(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string_view to_slang =
        (text == "facepalm") ? "fp" :
        (text == "shrug") ? "idk" :
        (text == "foldarms") ? "fold" :
        (text == "fa") ? "fold" :
        (text == "stubborn") ? "fold" : text;
    const std::string action = '/' + std::string(to_slang);

    if (pPeer->netid == 0 || pPeer->recent_worlds.empty() || (pPeer->state & S_INVISIBLE) == S_INVISIBLE)
    {
        send_varlist(event.peer, { "OnAction", action }, pPeer->netid);
        return;
    }
    // @note everyone in the world sees the emote, not just you
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        send_varlist(&p, { "OnAction", action }, pPeer->netid);
    });
}