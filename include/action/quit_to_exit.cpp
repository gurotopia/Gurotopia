#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    auto &peer = _peer[event.peer];

    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return; // @note peer was not in a world, therefore nothing to exit from.

    packet::create(*event.peer, true, 0, { "OnSetPos", std::vector<float>{-1, -1} });

    if (--it->second.visitors <= 0) worlds.erase(it); // @note take 1, and if result is 0, delete memory copy of world.

    peer->netid = 0; // this will fix any packets being sent outside of world; this can also be used to check if peer is not in a world.

    std::string &prefix = peer->prefix;
    const char* message = std::format("`5<`{}{}`` left, `w{}`` others here>``", prefix, peer->ltoken[0], it->second.visitors).c_str();
    const char* netid = std::format("netID|{}\n", peer->netid).c_str();
    const char* pId = std::format("pId|{}\n", peer->user_id).c_str(); // @note this is found during OnSpawn 'eid', the value is the same for user_id.
    peers(event, PEER_SAME_WORLD, [&peer, message, netid, pId](ENetPeer& p) 
    {
        packet::create(p, false, 0, { "OnConsoleMessage", message });
        packet::create(p, false, 0, { "OnRemove", netid, pId }); // @todo
    });

    if (prefix == "2" || prefix == "c") prefix.front() = 'w';
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}