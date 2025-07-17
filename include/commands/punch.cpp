#include "pch.hpp"
#include "punch.hpp"

void punch(ENetEvent& event, const std::string_view text) 
{
    std::string number{ text.substr(sizeof("punch ")-1) };
    if (number.empty()) return;

    auto &peer = _peer[event.peer];

    state_visuals(event, 
    {
        .type = 0x14 | ((0x808000 + stoi(number)) << 8), // @note 0x8080{}14
        .netid = _peer[event.peer]->netid,
        .pos = { peer->pos[0] * 32, peer->pos[1] * 32 },
        .speed = { 250, 800 } // @todo these are just random numbers I put. for my future-self/contributors please add the proper numbers.
    });
}