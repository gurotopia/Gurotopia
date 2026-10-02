#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "warpto.hpp"

void warpto(ENetEvent& event, const std::string_view text)
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
        on::ConsoleMessage(event.peer, "`4Usage: /warpto {player}``");
        return;
    }

    std::string lower_target = arg;
    for (char &c : lower_target) c = std::tolower(static_cast<unsigned char>(c));

    std::string target_world{};
    ::pos target_pos{};
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget || pTarget->netid == 0) return;

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name == lower_target) { target_world = pTarget->recent_worlds.back(); target_pos = pTarget->pos; }
    });

    if (target_world.empty())
    {
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not online or not in a world.``", arg));
        return;
    }

    if (target_world == pPeer->recent_worlds.back())
    {
        pPeer->pos = target_pos; // @note same world: just move there
        peers(target_world, PEER_SAME_WORLD, [&](ENetPeer &p) { send_varlist(&p, { "OnSetPos", CL_Vec2f{ target_pos.x, target_pos.y } }, pPeer->netid); });
        on::ConsoleMessage(event.peer, std::format("`2Warped to `w{}```.``", arg));
        return;
    }

    send_action(*event.peer, "log", std::format("msg|Warping to `5{}``...", target_world));
    action::quit_to_exit(event, "", true);
    arrive_at(static_cast<int>(pPeer->user_id), target_pos); // @note land where they stand
    action::join_request(event, "", target_world);
}
