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
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    try
    {
        peer->skin_color = (u_int)stoul(id);
        on::SetClothing(*event.peer);
    }
    catch (const std::logic_error &le) {} // @note std::invalid_argument std::out_of_range
}
