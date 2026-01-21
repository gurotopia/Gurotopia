#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "punch.hpp"

u_char get_punch_id(u_int item_id)
{
    switch (item_id)
    {
        case 138: case 2976: return 1; // @note https://growtopia.fandom.com/wiki/Mods/Eye_Beam
        case 366: return 2;
        case 472: return 3;
        case 594: return 4;
        case 768: return 5;
        case 900: return 6;

        case 930: return 8; // @todo or 12

        case 1204: return 10;
        case 1738: return 11;
        case 1484: return 12;

        case 3066: case 5206: case 7504: case 10288: return 17; // @note https://growtopia.fandom.com/wiki/Mods/Fire_Hose
        case 2636: case 2908: case 3070: case 3108: case 3466: return 29; // @note https://growtopia.fandom.com/wiki/Mods/Slasher

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
    ::peer *peer = static_cast<::peer*>(event.peer->data);


    try
    {
        peer->punch_effect = stoi(id);
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