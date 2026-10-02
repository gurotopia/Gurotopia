#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "swap.hpp"

/* /swap {player} -> trade positions with someone in your world */
void swap_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /swap {player}``");
        return;
    }

    std::string lower_target = arg;
    for (char &c : lower_target) c = std::tolower(static_cast<unsigned char>(c));

    bool found = false;
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget) return;

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name != lower_target) return;
        if (pTarget->user_id == pPeer->user_id) return;

        found = true;

        const ::pos mine = pPeer->pos;
        const ::pos theirs = pTarget->pos;

        pPeer->pos = theirs;
        pTarget->pos = mine;

        send_varlist(event.peer, { "OnSetPos", CL_Vec2f{ theirs.x, theirs.y } }, pPeer->netid);
        send_varlist(&p, { "OnSetPos", CL_Vec2f{ mine.x, mine.y } }, pTarget->netid);

        on::ConsoleMessage(&p, std::format("`5`w{}`` swapped places with you.``", pPeer->growid));
        on::ConsoleMessage(event.peer, std::format("`2Swapped places with `w{}``.``", pTarget->growid));
    });

    if (!found)
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not in this world.``", arg));
}
