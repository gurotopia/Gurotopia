#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "kick.hpp"

void kick(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /kick {player}``");
        return;
    }

    std::string lower_target = arg;
    for (char &c : lower_target) c = std::tolower(static_cast<unsigned char>(c));

    bool found = false;
    ENetPeer *kicked = nullptr;
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget) return;

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name != lower_target) return;

        if (pTarget->role >= pPeer->role)
        {
            on::ConsoleMessage(event.peer, "`4You cannot kick someone of equal or higher rank.``");
            found = true;
            return;
        }

        found = true;
        kicked = &p;
    });

    if (kicked) // @note after the loop: leaving changes the list we were walking through
    {
        ::peer *pTarget = static_cast<::peer*>(kicked->data);
        on::ConsoleMessage(kicked, "`4You have been kicked from this world.``");
        ENetEvent ev{};
        ev.peer = kicked;
        action::quit_to_exit(ev, "", false); // @note back to the world menu, still connected
        on::ConsoleMessage(event.peer, std::format("`2Kicked `w{}`` from the world.``", pTarget->growid));
    }

    if (!found)
    {
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not in this world.``", arg));
        return;
    }
}
