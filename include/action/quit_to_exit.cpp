#include "pch.hpp"
#include "network/packet.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

void quit_to_exit(ENetEvent event, const std::string& header, bool skip_selection = false) 
{
    auto &peer = _peer[event.peer];
    if (!peer->ready_exit) return; // @todo investigating action|quit_to_exit being called 2-3 times in a row...?
    peer->ready_exit = false;
    world &world = worlds[peer->recent_worlds.back()];
    --world.visitors;
    std::string& prefix = peer->prefix;
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        gt_packet(p, false, 0, {
            "OnConsoleMessage", 
            std::format("`5<`{}{}`` left, `w{}`` others here>``", prefix, peer->ltoken[0], world.visitors).c_str()
        });
        gt_packet(p, true, 0, {
            "OnRemove", 
            std::format("netID|{}\npId|\n", peer->netid).c_str()
        });
    });
    if (world.visitors <= 0) {
        worlds.erase(peer->recent_worlds.back());
    }
    if (prefix == "2" || prefix == "c") prefix = "w";
    peer->netid = -1; // this will fix any packets being sent outside of world
    if (!skip_selection) OnRequestWorldSelectMenu(event);
}