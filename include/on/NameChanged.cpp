#include "database/peer.hpp"
#include "network/packet.hpp"
#include "NameChanged.hpp"

void OnNameChanged(ENetEvent event) {
    gt_packet(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("`{}{}``", _peer[event.peer]->prefix, _peer[event.peer]->ltoken[0]).c_str()
    });
}