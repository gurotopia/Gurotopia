#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

void action::quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    auto &peer = _peer[event.peer];

    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return; // @note peer was not in a world, therefore nothing to exit from.

    packet::create(*event.peer, true, 0, { "OnSetPos", std::vector<float>{-1, -1} }); // @note idk why real growtopia does this.
    packet::create(*event.peer, true, 0, { "OnRemove", std::format("netID|{}\npId|\n", peer->netid).c_str() }); // @todo 

    if (--it->second.visitors <= 0) worlds.erase(it); // @note take 1, and if result is 0, delete memory copy of world.

    std::string &prefix = peer->prefix;
    if (prefix == "2" || prefix == "c") prefix.front() = 'w';

    peer->netid = 0; // this will fix any packets being sent outside of world; this can also be used to check if peer is not in a world.

    peers(event, PEER_SAME_WORLD, [&peer, &it, &prefix](ENetPeer& p) 
    {
        packet::create(p, false, 0, {
            "OnConsoleMessage", 
            std::format("`5<`{}{}`` left, `w{}`` others here>``", prefix, peer->ltoken[0], it->second.visitors).c_str()
        });
    });
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}