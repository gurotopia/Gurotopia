#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "deleteworld.hpp"

/* /deleteworld {name} -> removes a world from the database entirely */
void deleteworld(ENetEvent& event, const std::string_view text)
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

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /deleteworld {name}``");
        return;
    }

    for (char &c : arg) c = std::toupper(static_cast<unsigned char>(c));

    if (arg == pPeer->recent_worlds.back())
    {
        on::ConsoleMessage(event.peer, "`4Leave the world before deleting it.``");
        return;
    }

    // drop the in-memory copy first so its destructor doesn't rewrite the row
    if (auto it = std::ranges::find(worlds, arg, &::world::name); it != worlds.end())
    {
        if (it->visitors > 0)
        {
            on::ConsoleMessage(event.peer, "`4Somebody is still in that world.``");
            return;
        }
        { std::iter_swap(it, worlds.end() - 1); worlds.pop_back(); } // @note move to the end first so the world saves itself
    }

    ::hStmt hStmt{ "DELETE FROM world WHERE name = ?" };
    MYSQL_BIND param = make_bind_in(arg);
    hStmt.bind_param(&param);
    hStmt.execute();

    on::ConsoleMessage(event.peer,
        std::format("`2Deleted `w{}``. It will regenerate fresh if entered again.``", arg));
}
