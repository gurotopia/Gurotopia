#include "pch.hpp"
#include "ping_reply.hpp"

void ping_reply(ENetEvent& event, state state)
{
    printf("ping requested from %s\n", _peer[event.peer]->ltoken[0].c_str());
}