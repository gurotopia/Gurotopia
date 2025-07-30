#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "tools/string.hpp"
#include "skin.hpp"

void skin(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("skin ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /skin `w{id}``" });
        return;
    }
    std::string id{ text.substr(sizeof("skin ")-1) };

    if (atoi(id.c_str()) == 0) // @note so we can use negative skin colors
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`4Invalid input. ``id must be a `wnumber``."
        });
        return;
    }

    _peer[event.peer]->skin_color = stoi(id);
    on::SetClothing(event);
}