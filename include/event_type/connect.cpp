#include "pch.hpp"
#include "action/quit.hpp"
#include "connect.hpp"

void _connect(ENetEvent& event) 
{
    if (peers(event, PEER_ALL).size() > server->peerCount) 
    {
        packet::action(*event.peer, "log", 
            std::format(
                "msg|`4SERVER OVERLOADED`` : Sorry, our servers are currently at max capacity with {} online, please try later. We are working to improve this!",
                server->peerCount
            ));
        packet::action(*event.peer, "logon_fail", ""); // @note triggers action|quit on client.
    }

    constexpr std::array<enet_uint8, 4zu> connect_data = { 01 };
    enet_peer_send(event.peer, 0, enet_packet_create(connect_data.data(), connect_data.size(), ENET_PACKET_FLAG_RELIABLE));

    std::shared_ptr<peer> peer_data = std::make_shared<peer>();
    event.peer->data = peer_data.get();
    _peer.emplace(event.peer, std::move(peer_data));
}
