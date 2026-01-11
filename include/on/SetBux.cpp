#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    signed &gems = _peer[event.peer]->gems;
    static constexpr int signed_max = std::numeric_limits<signed>::max();
    gems = std::clamp(gems, 0, signed_max);

    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        gems,
        1,
        1
    });
}
