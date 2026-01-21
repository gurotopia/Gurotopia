#include "pch.hpp"
#include "NameChanged.hpp"

void on::NameChanged(ENetEvent& event) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("`{}{}``", peer->prefix, peer->ltoken[0]).c_str()
    });
}