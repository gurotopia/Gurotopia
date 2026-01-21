#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    static constexpr int signed_max = std::numeric_limits<signed>::max();
    peer->gems = std::clamp(peer->gems, 0, signed_max);

    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        peer->gems,
        1,
        1
    });
}
