#include "pch.hpp"
#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "action/trade.hpp"
#include "quit_to_exit.hpp"

void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    if (event.peer == nullptr || event.peer->data == nullptr) return;
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->netid == 0) { trade_end_for(*event.peer); return; }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) { trade_end_for(*event.peer); return; }

    std::string message = std::format("`5<{} left, `w{}`` others here>``", pPeer->display_growid, world->visitors-1);
    std::string netid = std::format("netID|{}\n", pPeer->netid);
    std::string pId = std::format("pId|{}\n", pPeer->user_id); // @note this is found during OnSpawn 'eid', the value is the same for user_id.
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, pPeer, message, netid, pId](ENetPeer& peer) 
    {
        ::peer *pOthers = static_cast<::peer*>(peer.data);
        if (pOthers->user_id == pPeer->user_id) return;

        on::ConsoleMessage(&peer, message, 100);
        send_action(peer, "play_sfx", "file|audio/door_shut.wav\ndelayMS|0");
        trade_partner_left(peer, *pPeer);
        send_varlist(&peer, { "OnRemove", netid, pId }); // @todo
    });

    pPeer->trade = {};
    if (--world->visitors <= 0) worlds.erase(world); // @note take 1, and if result is 0, delete memory copy of world.
    pPeer->netid = 0; // this will fix any packets being sent outside of world; this can also be used to check if peer is not in a world.

    pPeer->display_growid = std::format("`w{}``", pPeer->growid);
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}