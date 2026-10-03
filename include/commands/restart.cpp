#include "pch.hpp"
#include <csignal> // @note std::signal / SIGINT / SIGTERM (needed on Linux)
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "restart.hpp"

/* /restart -> warns everyone, saves, and shuts the server down cleanly */
void restart(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg != "confirm")
    {
        on::ConsoleMessage(event.peer,
            "`4This will shut the server down. Type `w/restart confirm`` to go ahead.``");
        return;
    }

    const std::string line = std::format(
        "`4** `wServer shutting down`` `4** `oSaving worlds, back shortly.``");

    peers("", peer_condition::PEER_ALL, [&line](ENetPeer &p)
    {
        on::ConsoleMessage(&p, line);
    });

    // save every loaded world before we go
    for (::world &w : worlds) w.save();

    printf("[restart] requested by %s\n", pPeer->growid.c_str());
    raise(SIGINT); // @note same path as pressing Ctrl+C
}
