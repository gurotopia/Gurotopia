#include "pch.hpp"
#include "SetClothing.hpp"

void on::SetClothing(ENetPeer &peer)
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);

    packet::create(peer, true, 0, {
        "OnSetClothing", 
        std::vector<float>{pPeer->clothing[hair], pPeer->clothing[shirt], pPeer->clothing[legs]}, 
        std::vector<float>{pPeer->clothing[feet], pPeer->clothing[face], pPeer->clothing[hand]}, 
        std::vector<float>{pPeer->clothing[back], pPeer->clothing[head], pPeer->clothing[charm]}, 
        (pPeer->state & S_GHOST) ? -140 : pPeer->skin_color,
        std::vector<float>{pPeer->clothing[ances], 0.0f, 0.0f}
    });

    ::state state
    {
        .type = 0x14 | ((0x808000 + pPeer->punch_effect) << 8), // @note 0x8080{}14 - PACKET_SET_CHARACTER_STATE
        .netid = pPeer->netid,
        .count = 125.0f, // @note gtnoob has this as 'waterspeed'
        .id = pPeer->state, // @note 04 invisible (only eyes/mouth), 08 no arms, 16 no face, 32 invisible (only legs/arms), 64 devil horns, 128 angel halo, 2048 frozen, 4096 gray skin?,8192 ducttape, 16384 Onion effect, 32768 stars effect, 65536 zombie, 131072 hit by lava, 262144 shadow effect, 524288 irradiated effect, 1048576 spotlight, 2097152 pineapple thingy
        .pos = ::pos{ 1200.0f, 200.0f },
        .speed = ::pos{ 250.0f, 1000.0f },
        .punch = ::pos{ pPeer->hair_color, 0x00000000 }
    };
    state_visuals(peer, std::move(state)); // @todo handle for 'p'
}