#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "list.hpp"

/* /list -> everyone online and which world they're in */
void list_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::vector<std::string> lines;
    peers("", peer_condition::PEER_ALL, [&lines](ENetPeer &p)
    {
        ::peer *pOthers = static_cast<::peer*>(p.data);
        if (!pOthers || pOthers->growid.empty()) return;

        const std::string where = (pOthers->netid == 0 || pOthers->recent_worlds.back().empty())
            ? "`o(world menu)"
            : std::format("`oin `2{}``", pOthers->recent_worlds.back());

        const char *tag = (pOthers->role >= DEVELOPER) ? "`6" : (pOthers->role >= MODERATOR) ? "`5" : "`w";

        lines.emplace_back(std::format("{}{}`` {}", tag, pOthers->growid, where));
    });

    if (lines.empty())
    {
        on::ConsoleMessage(event.peer, "`4Nobody is online.``");
        return;
    }

    on::ConsoleMessage(event.peer, std::format("`o--- `2{} online`o ---", lines.size()));
    for (const std::string &line : lines)
        on::ConsoleMessage(event.peer, line);
}
