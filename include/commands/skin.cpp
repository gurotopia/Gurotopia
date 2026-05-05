#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "on/ConsoleMessage.hpp"

#include "skin.hpp"

void skin(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("skin ") - 1) 
    {
        on::ConsoleMessage(event.peer, "Usage: /skin `w{id}``");
        return;
    }
    std::string id{ text.substr(sizeof("skin ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    try
    {
        pPeer->skin_color = (u_int)stoul(id);
        on::SetClothing(*event.peer);
    }
    catch (const std::logic_error &le) {} // @note std::invalid_argument std::out_of_range
}
