#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    static constexpr int signed_max = std::numeric_limits<signed>::max();
    pPeer->gems = std::clamp(pPeer->gems, 0, signed_max);

    packet::create(*event.peer, false, 0, {
        "OnSetBux",
        pPeer->gems,
        1,
        1
    });
}
