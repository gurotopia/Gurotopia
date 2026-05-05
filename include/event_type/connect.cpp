#include "pch.hpp"
#include "action/quit.hpp"
#include "connect.hpp"

void _connect(ENetEvent& event) 
{
    if (peers().size() > host->peerCount) 
    {
        send_action(*event.peer, "log", 
            std::format(
                "msg|`4SERVER OVERLOADED`` : Sorry, our servers are currently at max capacity with {} online, please try later. We are working to improve this!",
                host->peerCount
            ));
        send_action(*event.peer, "logon_fail", ""); // @note triggers action|quit on client.
    }
    else 
    {
        const enet_uint8 connect[4] = { 0x01, 0x00, 0x00, 0x00 };
        enet_peer_send(event.peer, 0, enet_packet_create(connect, std::size(connect), ENET_PACKET_FLAG_RELIABLE));

        event.peer->data = new peer();
    }
}
