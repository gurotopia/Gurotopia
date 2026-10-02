#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "onVariant/SetClothing.hpp"
#include "freeze.hpp"

/* /freeze {player} -> toggles a player's ability to move (same world only) */
void freeze(ENetEvent& event, const std::string_view text)
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
        on::ConsoleMessage(event.peer, "`4Usage: /freeze {player}``");
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

        if (pTarget->role >= pPeer->role && pTarget->user_id != pPeer->user_id)
        {
            on::ConsoleMessage(event.peer, "`4You cannot freeze someone of equal or higher rank.``");
            found = true;
            return;
        }

        found = true;
        pTarget->frozen = !pTarget->frozen;

        if (pTarget->frozen) pTarget->state |= S_FROZEN;
        else                 pTarget->state &= ~S_FROZEN;
        on::SetClothing(p);
        send_varlist(&p, { "OnSetFreezeState", pTarget->frozen ? 1u : 0u }, pTarget->netid);

        on::ConsoleMessage(&p, pTarget->frozen
            ? "`4You have been frozen.``"
            : "`2You can move again.``");

        on::ConsoleMessage(event.peer, std::format("`2`w{}`` is now {}.``",
            pTarget->growid, pTarget->frozen ? "`4frozen``" : "`2unfrozen``"));
    });

    if (!found)
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not in this world.``", arg));
}
