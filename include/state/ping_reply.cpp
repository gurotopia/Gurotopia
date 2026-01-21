#include "pch.hpp"
#include "ping_reply.hpp"

void ping_reply(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    std::printf("ping requested from %s\n", peer->ltoken[0].c_str());
}