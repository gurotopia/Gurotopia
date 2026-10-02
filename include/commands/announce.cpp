#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "announce.hpp"

/* /announce {message} -> server-wide broadcast from staff */
void announce(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::string message{ text };
    if (const std::size_t sp = message.find(' '); sp != std::string::npos) message = message.substr(sp + 1);
    else message.clear();

    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) message.pop_back();

    if (message.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /announce {message}``");
        return;
    }

    const std::string line =
        std::format("`5** `wServer Announcement`` `5from `w{}`` **`` `o{}``",
                    pPeer->growid, message);

    peers("", peer_condition::PEER_ALL, [&line](ENetPeer &p)
    {
        on::ConsoleMessage(&p, line);
    });
}
