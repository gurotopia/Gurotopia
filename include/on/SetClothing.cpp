#include "pch.hpp"
#include "SetClothing.hpp"

void on::SetClothing(ENetPeer &p)
{
    auto &peer = _peer[&p];
    
    packet::create(p, true, 0, {
        "OnSetClothing", 
        std::vector<float>{peer->clothing[hair], peer->clothing[shirt], peer->clothing[legs]}, 
        std::vector<float>{peer->clothing[feet], peer->clothing[face], peer->clothing[hand]}, 
        std::vector<float>{peer->clothing[back], peer->clothing[head], peer->clothing[charm]}, 
        (peer->state & S_GHOST) ? -140 : peer->skin_color,
        std::vector<float>{peer->clothing[ances], 0.0f, 0.0f}
    });

    ::state state
    {
        .type = 0x14 | ((0x808000 + peer->punch_effect) << 8), // @note 0x8080{}14
        .netid = peer->netid,
        .count = 43.75f, // @note gtnoob has this as 'waterspeed'
        .id = peer->state, // @note 04 invisible (only eyes/mouth), 08 no arms, 16 no face, 32 invisible (only legs/arms), 64 devil horns, 128 angel halo, 2048 frozen, 4096 gray skin?,8192 ducttape, 16384 Onion effect, 32768 stars effect, 65536 zombie, 131072 hit by lava, 262144 shadow effect, 524288 irradiated effect, 1048576 spotlight, 2097152 pineapple thingy
        .pos = { 1200.0f, 200.0f },
        .speed = { 250.0f, 1000.0f },
        .punch = { 0x1fffefff, 0x00000000 }
    };
    state_visuals(p, std::move(state)); // @todo handle for 'p'
}