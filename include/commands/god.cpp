#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "godmode.hpp"
#include "god.hpp"

/* /god -> toggles immunity to lava and other damage */
void god(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    pPeer->god_mode = !pPeer->god_mode;
    god_items_changed(event);
    pPeer->pain_hp = 10; // top up in case they were mid-burn

    on::ConsoleMessage(event.peer,
        pPeer->god_mode
            ? "`2God mode `wON``. Nothing can hurt you.``"
            : "`4God mode `wOFF``.``");
}
