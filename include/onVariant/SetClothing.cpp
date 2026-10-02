#include "pch.hpp"
#include "commands/useitems.hpp"
#include "SetClothing.hpp"

/* /invis: your own game shows only eyes, mouth, name and flag (no clothes, see-through skin).
 * With /ghost on too the game would tint the face purple, so then it uses see-through skin instead.
 * State flag 0x4 = The One Ring look: the game draws only eyes and mouth over the normal skin.
 * The invisible marker itself stays on the server; sent to the game it tints the face purple */

/* sends "who" appearance to the single client "to" */
void on::SetClothing(ENetPeer &to, ::peer &who)
{
    const bool invis = (who.state & S_INVISIBLE) == S_INVISIBLE;
    auto worn = [&](auto slot) { auto v = who.clothing[slot]; if (invis) v = 0; return v; };

    send_varlist(&to, {
        "OnSetClothing",
        CL_Vec3f{worn(clothing::HAIR), worn(clothing::SHIRT), worn(clothing::LEGS)},
        CL_Vec3f{worn(clothing::FEET), worn(clothing::FACE), worn(clothing::HAND)},
        CL_Vec3f{worn(clothing::BACK), worn(clothing::HEAD), worn(clothing::CHARM)},
        invis ? ((who.state & S_GHOST) ? 0xFFFFFF00u : 0xFFFFFFFFu) : (who.state & S_GHOST) ? -140 : (who.frozen || (who.state & S_FROZEN)) ? 0xFFB47880u : item_skin(who),
        CL_Vec3f{worn(clothing::ANCES), 0.0f, 0.0f}
    }, who.netid);

    ::gamePacket gamePacket {
        .type = 0x14 | ((0x808000 + who.punch_effect) << 8),
        .netid = who.netid,
        .count = 125.0f,
        .id = (who.state & ~S_DOUBLE_JUMP & ~S_INVISIBLE) | ((invis && !(who.state & S_GHOST)) ? 0x4 : 0) | double_jump_state(who) | ((who.curse_until > std::time(nullptr)) ? 0x3000 : 0) | item_effect_state(who),
        .pos = ::pos{ 1200.0f, 200.0f },
        .speed = item_speed(who),
        .punch = ::pos{ who.hair_color, 0x00000000u }
    };
    send_data(to, compress_state(gamePacket));
}

void on::SetClothing(ENetPeer &peer)
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);
    const bool invis = (pPeer->state & S_INVISIBLE) == S_INVISIBLE;
    auto worn = [&](auto slot) { auto v = pPeer->clothing[slot]; if (invis) v = 0; return v; };

    send_varlist(&peer, {
        "OnSetClothing",
        CL_Vec3f{worn(clothing::HAIR), worn(clothing::SHIRT), worn(clothing::LEGS)},
        CL_Vec3f{worn(clothing::FEET), worn(clothing::FACE), worn(clothing::HAND)},
        CL_Vec3f{worn(clothing::BACK), worn(clothing::HEAD), worn(clothing::CHARM)},
        invis ? ((pPeer->state & S_GHOST) ? 0xFFFFFF00u : 0xFFFFFFFFu) : (pPeer->state & S_GHOST) ? -140 : (pPeer->frozen || (pPeer->state & S_FROZEN)) ? 0xFFB47880u : item_skin(*pPeer),
        CL_Vec3f{worn(clothing::ANCES), 0.0f, 0.0f}
    }, pPeer->netid);

    ::gamePacket gamePacket {
        .type = 0x14 | ((0x808000 + ((pPeer->clothing[clothing::HAND] == 12412) ? 0x200000 : 0) + pPeer->punch_effect) << 8), // @note 0x8080{}14 - PACKET_SET_CHARACTER_STATE
        .netid = pPeer->netid,
        .count = 125.0f, // @note gtnoob has this as 'waterspeed'
        .id = (pPeer->state & ~S_DOUBLE_JUMP & ~S_INVISIBLE) | ((invis && !(pPeer->state & S_GHOST)) ? 0x4 : 0) | double_jump_state(*pPeer) | ((pPeer->curse_until > std::time(nullptr)) ? 0x3000 : 0) | item_effect_state(*pPeer),
        .pos = ::pos{ 1200.0f, 200.0f }, // @todo magic numbers
        .speed = item_speed(*pPeer), // @todo magic numbers
        .punch = ::pos{ pPeer->hair_color, 0x00000000u } // @todo can this even be unsigned?
    };
    state_visuals(peer, std::move(gamePacket)); // @todo handle for 'p'

    /* the state above already reaches everyone, but OnSetClothing (skin colour, clothes) only went to this player */
    if (pPeer->netid != 0 && (pPeer->state & S_INVISIBLE) != S_INVISIBLE)
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, pPeer](ENetPeer &other)
        {
            if (&other != &peer) on::SetClothing(other, *pPeer);
        });
}

/* minimal version for bots: default clothing, just needs a netid */
void on::SetClothing(ENetPeer &to, int netid, u_int skin, int state, const std::array<float, 10ull> *cloth)
{
    send_varlist(&to, {
        "OnSetClothing",
        CL_Vec3f{cloth ? (*cloth)[0] : 0.0f, cloth ? (*cloth)[1] : 0.0f, cloth ? (*cloth)[2] : 0.0f},
        CL_Vec3f{cloth ? (*cloth)[3] : 0.0f, cloth ? (*cloth)[4] : 0.0f, cloth ? (*cloth)[5] : 0.0f},
        CL_Vec3f{cloth ? (*cloth)[6] : 0.0f, cloth ? (*cloth)[7] : 0.0f, cloth ? (*cloth)[8] : 0.0f},
        (state & S_GHOST) ? (u_int)(-140) : skin,
        CL_Vec3f{cloth ? (*cloth)[9] : 0.0f, 0.0f, 0.0f}
    }, netid);

    ::gamePacket gp{
        .type = 0x14 | (0x808000 << 8),
        .netid = netid,
        .count = 125.0f,
        .id = state,
        .pos = ::pos{ 1200.0f, 200.0f },
        .speed = ::pos{ 250.0f, 1000.0f },
        .punch = ::pos{ 0.0f, 0.0f }
    };
    send_data(to, compress_state(gp));
}