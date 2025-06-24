#include "pch.hpp"
#include "network/packet.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

void quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection = false) 
{
    auto &peer = _peer[event.peer];

    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return; // @note peer was not in a world, therefore nothing to exit from.

    --it->second.visitors;
    std::string& prefix = peer->prefix;
    peers(event, PEER_SAME_WORLD, [&peer, &it, prefix](ENetPeer& p) 
    {
        gt_packet(p, false, 0, {
            "OnConsoleMessage", 
            std::format("`5<`{}{}`` left, `w{}`` others here>``", prefix, peer->ltoken[0], it->second.visitors).c_str()
        });
        gt_packet(p, true, 0, { "OnRemove", std::format("netID|{}\npId|\n", peer->netid).c_str() });
    });
    if (it->second.visitors <= 0) worlds.erase(it);
    if (prefix == "2" || prefix == "c") prefix = "w";
    peer->netid = -1; // this will fix any packets being sent outside of world
    if (!skip_selection) OnRequestWorldSelectMenu(event);
}