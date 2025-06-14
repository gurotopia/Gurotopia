#include "pch.hpp"
#include "network/packet.hpp"
#include "refresh_item_data.hpp"

void refresh_item_data(ENetEvent event, const std::string& header) 
{
    gt_packet(*event.peer, false, 0, {
        "OnConsoleMessage",
        "One moment, updating item data..."
    });
    enet_peer_send(event.peer, 0, enet_packet_create(im_data.data(), im_data.size(), ENET_PACKET_FLAG_RELIABLE));
}