#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "quit_to_exit.hpp"

#include <format>

void quit_to_exit(ENetEvent event, const std::string& header, bool skip_selection = false) 
{
    if (!_peer[event.peer]->ready_exit) return; // @todo investigating action|quit_to_exit being called 2-3 times in a row...?
    _peer[event.peer]->ready_exit = false;
    --worlds[_peer[event.peer]->recent_worlds.back()].visitors;
    peers(ENET_PEER_STATE_CONNECTED, [&](ENetPeer& p) 
    {
        if (!_peer[&p]->recent_worlds.empty() && !_peer[event.peer]->recent_worlds.empty() && 
            _peer[&p]->recent_worlds.back() == _peer[event.peer]->recent_worlds.back()) 
        {
            gt_packet(p, false, 0, {
                "OnConsoleMessage", 
                std::format("`5<`w{}`` left, `w{}`` others here>``", _peer[event.peer]->ltoken[0], worlds[_peer[event.peer]->recent_worlds.back()].visitors).c_str()
            });
            gt_packet(p, true, 0, {
                "OnRemove", 
                std::format("netID|{}\npId|\n", _peer[event.peer]->netid).c_str()
            });
        }
    });
    if (worlds[_peer[event.peer]->recent_worlds.back()].visitors <= 0) {
        worlds.erase(_peer[event.peer]->recent_worlds.back());
    }
    _peer[event.peer]->post_enter.unlock();
    _peer[event.peer]->netid = -1; // this will fix any packets being sent outside of world
    if (!skip_selection) OnRequestWorldSelectMenu(event);
}