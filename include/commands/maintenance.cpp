#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "maintenance.hpp"

bool gMaintenance = false;

/* /maintenance -> blocks non-staff from logging in */
void maintenance(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    gMaintenance = !gMaintenance;

    const std::string line = gMaintenance
        ? "`4** Server is going into maintenance **``"
        : "`2** Maintenance mode is off **``";

    peers("", peer_condition::PEER_ALL, [&line](ENetPeer &p)
    {
        on::ConsoleMessage(&p, line);
    });

    if (gMaintenance) // @note everyone who isn't staff is disconnected right away
        peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
        {
            const ::peer *t = static_cast<::peer*>(p.data);
            if (!t || &p == event.peer || t->role >= MODERATOR) return;
            on::ConsoleMessage(&p, "`4The server is going into maintenance.`` You've been disconnected - try again later.");
            enet_peer_disconnect_later(&p, 0);
        });

    on::ConsoleMessage(event.peer, gMaintenance
        ? "`4Maintenance mode `wON``. Only staff can log in.``"
        : "`2Maintenance mode `wOFF``.``");
}
