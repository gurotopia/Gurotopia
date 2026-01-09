#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "punch.hpp"

u_char get_punch_id(u_int item_id)
{
    switch (item_id)
    {
        case 138: case 2976: return 1;
        case 366: return 2;
        case 472: return 3;
        default: return 0;
    }
}

void punch(ENetEvent& event, const std::string_view text) 
{
    if (text.length() <= sizeof("punch ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /punch `w{id}``" });
        return;
    }
    std::string id{ text.substr(sizeof("punch ")-1) };
    try
    {
        _peer[event.peer]->punch_effect = stoi(id);
        on::SetClothing(*event.peer);
    }
    catch (const std::invalid_argument &ex)
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`4Invalid input. ``id must be a `wnumber``."
        });
    }
}