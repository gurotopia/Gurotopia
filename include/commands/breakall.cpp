#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "breakall.hpp"

/* /breakall -> toggles one-hit breaking on every block */
void breakall(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    pPeer->instant_break = !pPeer->instant_break;

    on::ConsoleMessage(event.peer, pPeer->instant_break
        ? "`2One-hit breaking `wON``.``"
        : "`4One-hit breaking `wOFF``.``");
}
