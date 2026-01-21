#include "pch.hpp"
#include "SetClothing.hpp"

void on::SetClothing(ENetPeer &p)
{
    ::peer *_p = static_cast<::peer*>(p.data);

    packet::create(p, true, 0, {
        "OnSetClothing", 
        std::vector<float>{_p->clothing[hair], _p->clothing[shirt], _p->clothing[legs]}, 
        std::vector<float>{_p->clothing[feet], _p->clothing[face], _p->clothing[hand]}, 
        std::vector<float>{_p->clothing[back], _p->clothing[head], _p->clothing[charm]}, 
        (_p->state & S_GHOST) ? -140 : _p->skin_color,
        std::vector<float>{_p->clothing[ances], 0.0f, 0.0f}
    });

    ::state state
    {
        .type = 0x14 | ((0x808000 + _p->punch_effect) << 8), // @note 0x8080{}14 - PACKET_SET_CHARACTER_STATE
        .netid = _p->netid,
        .count = 125.0f, // @note gtnoob has this as 'waterspeed'
        .id = _p->state, // @note 04 invisible (only eyes/mouth), 08 no arms, 16 no face, 32 invisible (only legs/arms), 64 devil horns, 128 angel halo, 2048 frozen, 4096 gray skin?,8192 ducttape, 16384 Onion effect, 32768 stars effect, 65536 zombie, 131072 hit by lava, 262144 shadow effect, 524288 irradiated effect, 1048576 spotlight, 2097152 pineapple thingy
        .pos = ::pos{ 1200.0f, 200.0f },
        .speed = { 250.0f, 1000.0f },
        .punch = ::pos{ 0x1fffefff, 0x00000000 }
    };
    state_visuals(p, std::move(state)); // @todo handle for 'p'
}