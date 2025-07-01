#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        _peer[event.peer]->gems,
        1,
        1
    });
}