#include "pch.hpp"
#include "punch.hpp"

void punch(ENetEvent& event, const std::string_view text) 
{
    std::string number{ text.substr(sizeof("punch ")-1) };
    if (number.empty()) return;

    auto &peer = _peer[event.peer];

    std::vector<std::byte> compress = compress_state({
        .type = 0x14 | ((0x808000 + stoi(number)) << 8),
        .netid = _peer[event.peer]->netid,
        .pos = { peer->pos[0] * 32, peer->pos[1] * 32 },
        .speed = { 300, 600 }
    });
    send_data(*event.peer, compress);
}