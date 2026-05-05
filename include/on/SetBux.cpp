#include "pch.hpp"
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->gems = std::clamp(pPeer->gems, 0, std::numeric_limits<signed>::max());

    send_varlist(event.peer, { "OnSetBux", pPeer->gems, 1, 1 });
}
