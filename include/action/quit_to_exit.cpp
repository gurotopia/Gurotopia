#include "pch.hpp"
#include <chrono>
#include <deque>
#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/trade.hpp"
#include "quit_to_exit.hpp"

namespace
{
/* someone left: the others see the "left" bubble first, and the avatar is removed a moment later */
struct pending_remove
{
    std::chrono::steady_clock::time_point at;
    ENetPeer *to;
    std::string world, netid, pid;
    int leaver;
};
std::deque<pending_remove> pending{};

void send_remove(const pending_remove &r)
{
    if (r.to->state != ENET_PEER_STATE_CONNECTED || r.to->data == nullptr) return;
    const ::peer *t = static_cast<::peer*>(r.to->data);
    if (t->netid == 0 || t->recent_worlds.back() != r.world) return; // @note they left too, nothing to remove
    send_varlist(r.to, { "OnRemove", r.netid, r.pid });
}
}

bool g_silent_move = false;

/* the same account is entering this world again: finish its pending removals first so the new avatar isn't removed */
void leave_flush(const std::string &world, int user_id)
{
    for (auto it = pending.begin(); it != pending.end(); )
    {
        if (it->leaver == user_id && it->world == world) { send_remove(*it); it = pending.erase(it); }
        else ++it;
    }
}

/* every loop of the server */
void leave_pump()
{
    const auto now = std::chrono::steady_clock::now();
    while (!pending.empty() && pending.front().at <= now)
    {
        send_remove(pending.front());
        pending.pop_front();
    }
}

void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->netid == 0) // @note not in a world (already left): nothing to leave, and the visitor count must never drop twice
    {
        if (!skip_selection) on::RequestWorldSelectMenu(event);
        return;
    }

    trade_end_for(*pPeer, "left the world"); // @note a trade can't outlive the world it started in

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end())
    {
        pPeer->netid = 0;
        if (!skip_selection) on::RequestWorldSelectMenu(event);
        return;
    }

    const bool hidden = (pPeer->state & S_INVISIBLE) == S_INVISIBLE || g_silent_move; // @note /invis players leave silently
    const std::string world_name = world->name;
    const std::string message = std::format("`5<{} left, `w{}`` others here>``", pPeer->display_growid, world->visitors > 0 ? world->visitors - 1 : 0);
    const std::string netid = std::format("netID|{}\n", pPeer->netid);
    const std::string pId = std::format("pId|{}\n", pPeer->user_id); // @note this is found during OnSpawn 'eid', the value is the same for user_id.
    const auto when = std::chrono::steady_clock::now() + std::chrono::milliseconds(1200);
    const int leaver_netid = pPeer->netid;

    if (!hidden)
        peers(world_name, PEER_SAME_WORLD, [&](ENetPeer& peer) 
        {
            ::peer *pOthers = static_cast<::peer*>(peer.data);
            if (pOthers->user_id == pPeer->user_id) return;

            on::ConsoleMessage(&peer, message);
            send_varlist(&peer, { "OnTalkBubble", leaver_netid, message, 1u }); // @note same bubble as when someone enters
            pending.push_back({ when, &peer, world_name, netid, pId, pPeer->user_id });
        });

    if (world->visitors > 0) --world->visitors;
    if (world->visitors == 0) { std::iter_swap(world, worlds.end() - 1); worlds.pop_back(); } // @note move to the end first so the world saves itself
    pPeer->netid = 0; // this will fix any packets being sent outside of world; this can also be used to check if peer is not in a world.

    if (pPeer->nickname.empty()) pPeer->display_growid = (pPeer->role >= DEVELOPER) ? std::format("`6@{}``", pPeer->growid) : (pPeer->role >= MODERATOR) ? std::format("`5@{}``", pPeer->growid) : std::format("`w{}``", pPeer->growid);
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}
