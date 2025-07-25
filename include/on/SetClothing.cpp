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

    state_visuals(event, 
    {
        .type = 0x14 | ((0x808000 + peer->punch_effect) << 8), // @note 0x8080{}14
        .netid = _peer[event.peer]->netid,
        .count = 127.0f,
        .id = 00, // @note playermod? double jump is 02
        .pos = { peer->pos[0] * 32, peer->pos[1] * 32 },
        .speed = { 248.0f, 992.0f },
        .punch = { 0x1fffefff } // @note eye color
    });
}