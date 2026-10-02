#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "database/world.hpp"
#include "summon.hpp"

/* /summon {player} -> brings a player to where YOU stand, from any world */
void summon(ENetEvent& event, const std::string_view text)
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
        on::ConsoleMessage(event.peer, "`4Usage: /summon {player}``");
        return;
    }

    std::string lower_target = arg;
    for (char &c : lower_target) c = std::tolower(static_cast<unsigned char>(c));

    const std::string my_world = pPeer->recent_worlds.back();
    const ::pos my_pos = pPeer->pos;

    bool found = false;
    ENetPeer *target_peer = nullptr;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget || pTarget->growid.empty()) return; // @note in a world or in the world menu, both count

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name == lower_target) target_peer = &p;
    });

    if (target_peer)
    {
        found = true;
        ENetPeer &p = *target_peer;
        ::peer *pTarget = static_cast<::peer*>(p.data);

        if (pTarget != pPeer && pTarget->role >= pPeer->role)
        {
            tell(event.peer, "`4You can't summon someone of equal or higher rank.``");
            return;
        }
        if (pTarget->curse_until > std::time(nullptr) && my_world != "HELL")
        {
            tell(event.peer, "`4That player is cursed - they can't leave HELL.``");
            return;
        }

        if (pTarget->netid != 0 && pTarget->recent_worlds.back() == my_world)
        {
            pTarget->pos = my_pos;
            send_varlist(&p, { "OnSetPos", CL_Vec2f{ my_pos.x, my_pos.y } }, pTarget->netid);
        }
        else
        {
            ENetEvent ev{};
            ev.peer = &p;
            arrive_at(pTarget->user_id, my_pos); // @note they appear where you stand, not at the door
            if (pTarget->netid != 0) action::quit_to_exit(ev, "", true); // @note in the world menu there's nothing to leave
            action::join_request(ev, "", my_world);
        }

        // an info popup like the event ones; it doesn't say who
        const std::string who = (pPeer->role >= DEVELOPER) ? "`6Dev``" : "`5Mod``";
        send_varlist(&p, { "OnAddNotification", "interface/atomic_button.rttex", std::format("`wYou were summoned by a {}!``", who), "audio/hub_open.wav", 0u }); // @note popup without the event picture
        on::ConsoleMessage(&p, std::format("`5You were summoned by a {}.``", who));

        on::ConsoleMessage(event.peer, std::format("`2Summoned `w{}``.``", pTarget->growid));
    }

    if (!found)
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not online.``", arg));
}
