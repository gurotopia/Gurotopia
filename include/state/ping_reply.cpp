#include "pch.hpp"
#include "ping_reply.hpp"

void ping_reply(ENetEvent& event, state state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    std::printf("ping requested from %s\n", pPeer->growid.c_str());
}