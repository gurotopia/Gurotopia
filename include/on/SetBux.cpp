#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    signed &gems = _peer[event.peer]->gems;
    static constexpr int signed_max = std::numeric_limits<signed>::max();

    if (gems > signed_max) gems = signed_max;
    if (gems < 0) gems = 0;

    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        gems,
        1,
        1
    });
}
