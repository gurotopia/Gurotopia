#include "pch.hpp"
#include "movement.hpp"
#include "tile_change.hpp"
#include "tile_activate.hpp"
#include "item_activate.hpp"
#include "item_activate_object.hpp"
#include "ping_reply.hpp"
#include "disconnect.hpp"

#include "_gamePacket.hpp"

std::unordered_map<u_char, std::function<void(ENetEvent&, ::gamePacket)>> gamePacket_pool
{
    {0x00, std::bind(&movement, std::placeholders::_1, std::placeholders::_2)},
    {0x03, std::bind(&tile_change, std::placeholders::_1, std::placeholders::_2)},
    {0x07, std::bind(&tile_activate, std::placeholders::_1, std::placeholders::_2)},
    {0x0a, std::bind(&item_activate, std::placeholders::_1, std::placeholders::_2)},
    {0x0b, std::bind(&item_activate_object, std::placeholders::_1, std::placeholders::_2)},
    {0x15, std::bind(&ping_reply, std::placeholders::_1, std::placeholders::_2)},
    {0x1a, std::bind(&disconnect, std::placeholders::_1, std::placeholders::_2)},
    {0x12, [](ENetEvent &event, ::gamePacket gamePacket) // @note PACKET_SET_ICON_STATE: the "..." bubble while typing
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        if (!pPeer || pPeer->netid == 0) return;
        gamePacket.netid = pPeer->netid;
        state_visuals(*event.peer, std::move(gamePacket));
    }}
};