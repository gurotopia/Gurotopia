#include "pch.hpp"
#include "SetClothing.hpp"

void on::SetClothing(ENetEvent& event, ENetPeer* p)
{
    auto &peer = _peer[(p == nullptr) ? event.peer : p];
    
    packet::create((p == nullptr) ? *event.peer : *p, true, 0, {
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
        .count = 125.0f, // @note gtnoob has this as 'waterspeed'
        .id = peer->state, // @note 01 ghost, 02 double jump, 04 invisible (only eyes/mouth), 08 no arms, 16 no face, 32 invisible (only legs/arms), 64 devil horns, 128 angel halo, 2048 frozen, 4096 gray skin?,8192 ducttape, 16384 Onion effect, 32768 stars effect, 65536 zombie, 131072 hit by lava, 262144 shadow effect, 524288 irradiated effect, 1048576 spotlight, 2097152 pineapple thingy
        .pos = { peer->pos[0] * 32, peer->pos[1] * 32 },
        .speed = { 250.0f, 1000.0f }
    };
    state_visuals(event, std::move(state)); // @todo handle for 'p'
}