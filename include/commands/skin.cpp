#include "pch.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "skin.hpp"

void skin(ENetEvent& event, const std::string_view text)
{
    const std::string id{ text.substr(sizeof("skin ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    try
    {
        pPeer->skin_color = (u_int)stoul(id);
        on::SetClothing(*event.peer);
    }
    catch (const std::logic_error &le) {} // @note std::invalid_argument std::out_of_range
}
