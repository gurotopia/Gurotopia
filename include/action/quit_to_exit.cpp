#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    auto &peer = _peer[event.peer];

    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return; // @note peer was not in a world, therefore nothing to exit from.

    std::string &prefix = peer->prefix;
    std::string message = std::format("`5<`{}{}`` left, `w{}`` others here>``", prefix, peer->ltoken[0], it->second.visitors);
    std::string netid = std::format("netID|{}\n", peer->netid);
    std::string pId = std::format("pId|{}\n", peer->user_id); // @note this is found during OnSpawn 'eid', the value is the same for user_id.
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, message, netid, pId](ENetPeer& p) 
    {
        packet::create(p, false, 0, { "OnConsoleMessage", message.c_str() });
        packet::create(p, false, 0, { "OnRemove", netid.c_str(), pId.c_str() }); // @todo
    });

    if (--it->second.visitors <= 0) worlds.erase(it); // @note take 1, and if result is 0, delete memory copy of world.
    peer->netid = 0; // this will fix any packets being sent outside of world; this can also be used to check if peer is not in a world.

    if (prefix == "2" || prefix == "c") prefix.front() = 'w';
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}