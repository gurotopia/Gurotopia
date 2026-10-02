#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/items.hpp"
#include "search.hpp"

/* /search {text} -> lists items whose name contains that text, with their ids */
void search_cmd(ENetEvent& event, const std::string_view text)
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

    if (arg.length() < 2)
    {
        on::ConsoleMessage(event.peer, "`4Usage: /search {at least 2 characters}``");
        return;
    }

    std::string needle = arg;
    for (char &c : needle) c = std::tolower(static_cast<unsigned char>(c));

    std::size_t found = 0;
    for (const ::item &it : items)
    {
        if (it.raw_name.empty()) continue;

        std::string hay = it.raw_name;
        for (char &c : hay) c = std::tolower(static_cast<unsigned char>(c));

        if (hay.find(needle) == std::string::npos) continue;

        ++found;
        if (found > 30)
        {
            on::ConsoleMessage(event.peer, "`o...more matches, narrow your search.");
            break;
        }

        on::ConsoleMessage(event.peer,
            std::format("`w{}`` `o- id `2{}``  type `w{}``", it.raw_name, (int)it.id, (int)it.type));
    }

    if (found == 0)
        on::ConsoleMessage(event.peer, std::format("`4Nothing matching `w{}``.``", arg));
    else
        on::ConsoleMessage(event.peer, std::format("`o{} match{}.", found, found == 1 ? "" : "es"));
}
