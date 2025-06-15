#include "pch.hpp"
#include "network/packet.hpp"
#include "NameChanged.hpp"

void OnNameChanged(ENetEvent event) {
    auto &peer = _peer[event.peer];
    gt_packet(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("`{}{}``", peer->prefix, peer->ltoken[0]).c_str()
    });
}