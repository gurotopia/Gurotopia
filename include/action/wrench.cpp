#include "pch.hpp"
#include "database/peer.hpp"
#include "wrench.hpp"

#include "tools/string_view.hpp"

void wrench(ENetEvent event, const std::string& header) 
{
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes[3] == "netid" && !pipes[4].empty()/*empty netid*/)
    {
        const short netid = stoi(pipes[4]);
        peers(ENET_PEER_STATE_CONNECTED, [&](ENetPeer& p) 
        {
            if (!_peer[&p]->recent_worlds.empty() && !_peer[event.peer]->recent_worlds.empty() && 
                _peer[&p]->recent_worlds.back() == _peer[event.peer]->recent_worlds.back() &&
                _peer[&p]->netid == netid)
            {
                // @todo add wrench dialog
                
                return; // @note early exit
            }
        });
    }
}