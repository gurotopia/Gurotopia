#include "pch.hpp"
#include "SetBux.hpp"

#define signed_max 2147483647

void on::SetBux(ENetEvent& event)
{
    signed &gems = _peer[event.peer]->gems;

    if (gems > signed_max) gems = signed_max;
    if (gems < 0) gems = 0;

    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        gems,
        1,
        1
    });
}

#undef signed_max