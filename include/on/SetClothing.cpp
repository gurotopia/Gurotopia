#include "pch.hpp"
#include "SetClothing.hpp"

void on::SetClothing(ENetEvent& event)
{
    auto &peer = _peer[event.peer];
    
    packet::create(*event.peer, true, 0, {
        "OnSetClothing", 
        std::vector<float>{peer->clothing[hair], peer->clothing[shirt], peer->clothing[legs]}, 
        std::vector<float>{peer->clothing[feet], peer->clothing[face], peer->clothing[hand]}, 
        std::vector<float>{peer->clothing[back], peer->clothing[head], peer->clothing[charm]}, 
        peer->skin_color,
        std::vector<float>{peer->clothing[ances], 0.0f, 0.0f}
    });
}