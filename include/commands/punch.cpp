#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "punch.hpp"

u_char get_punch_id(u_int item_id)
{
    switch (item_id)
    {
        // @todo
        default: return 0;
    }
}

void punch(ENetEvent& event, const std::string_view text) 
{
    std::string number{ text.substr(sizeof("punch ")-1) };
    if (number.empty()) return;

    auto &peer = _peer[event.peer];

    peer->punch_effect = stoi(number);
    on::SetClothing(event);
}