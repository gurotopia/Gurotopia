#include "pch.hpp"
#include "NameChanged.hpp"

void on::NameChanged(ENetEvent& event) {
    auto &peer = _peer[event.peer];

    packet::create(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("`{}{}``", peer->prefix, peer->ltoken[0]).c_str()
    });
}