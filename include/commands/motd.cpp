#include "pch.hpp"
#include <fstream>
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "motd.hpp"

std::string gMotd{};

/* /motd            -> shows the current message of the day
 * /motd {text}     -> sets it (dev)
 * /motd clear      -> removes it
 */
void motd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg.empty())
    {
        if (gMotd.empty()) on::ConsoleMessage(event.peer, "`oNo message of the day is set.");
        else on::ConsoleMessage(event.peer, std::format("`5[MOTD]`` `o{}", gMotd));
        return;
    }

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can change the MOTD.``");
        return;
    }

    if (arg == "clear")
    {
        gMotd.clear();
        std::ofstream("motd.txt");
        on::ConsoleMessage(event.peer, "`2Message of the day cleared.``");
        return;
    }

    gMotd = arg;
    { std::ofstream ostrm("motd.txt"); ostrm << gMotd; }

    peers("", peer_condition::PEER_ALL, [](ENetPeer &p)
    {
        on::ConsoleMessage(&p, std::format("`5[MOTD]`` `o{}", gMotd));
    });

    on::ConsoleMessage(event.peer, "`2Message of the day updated.``");
}
