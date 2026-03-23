#include "pch.hpp"
#include "NameChanged.hpp"

void on::NameChanged(ENetEvent& event) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("`{}{}``", pPeer->prefix, pPeer->ltoken[0]).c_str()
    });
}