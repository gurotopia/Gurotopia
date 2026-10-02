#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/godmode.hpp"
#include "refresh_item_data.hpp"

void action::refresh_item_data(ENetEvent& event, const std::string& header) 
{
    on::ConsoleMessage(event.peer, "One moment, updating item data...");
    enet_peer_send(event.peer, 0, enet_packet_create(items_for(event).data(), items_for(event).size(), ENET_PACKET_FLAG_RELIABLE));
}