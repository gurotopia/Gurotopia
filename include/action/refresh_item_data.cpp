#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "refresh_item_data.hpp"

void action::refresh_item_data(ENetEvent& event, const std::string& header) 
{
    on::ConsoleMessage(event.peer, "One moment, updating item data...");
    enet_peer_send(event.peer, 0, enet_packet_create(im_data.data(), im_data.size(), ENET_PACKET_FLAG_RELIABLE));
}