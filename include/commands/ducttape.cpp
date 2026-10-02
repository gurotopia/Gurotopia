#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/world.hpp"
#include "ducttape.hpp"

/* /ducttape           -> toggles duct tape on yourself
 * /ducttape {player}  -> toggles it on someone in your world
 */
void ducttape(ENetEvent& event, const std::string_view text)
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

    // no argument: apply to yourself
    if (arg.empty())
    {
        pPeer->state ^= S_DUCT_TAPE;
        on::SetClothing(*event.peer);

        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p)
        {
            on::SetClothing(p, *pPeer);
        });

        on::ConsoleMessage(event.peer, (pPeer->state & S_DUCT_TAPE)
            ? "`2Duct tape applied.``"
            : "`2Duct tape removed.``");
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
            on::ConsoleMessage(event.peer, "`4You cannot duct tape someone of equal or higher rank.``");
            found = true;
            return;
        }

        found = true;
        pTarget->state ^= S_DUCT_TAPE;
        const bool taped = (pTarget->state & S_DUCT_TAPE) != 0;

        on::SetClothing(p, *pTarget);
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pTarget](ENetPeer &q) { ::peer *o = static_cast<::peer*>(q.data); if (o && o->user_id != pTarget->user_id) on::SetClothing(q, *pTarget); });

        on::ConsoleMessage(&p, taped
            ? "`4Somebody duct taped your mouth shut!``"
            : "`2The duct tape was removed.``");

        on::ConsoleMessage(event.peer, std::format("`2`w{}`` is now {}.``",
            pTarget->growid, taped ? "`4duct taped``" : "`2free to speak``"));
    });

    if (!found)
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not in this world.``", arg));
}
