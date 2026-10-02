#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "renameworld.hpp"

/* /renameworld {newname} -> renames the world you're standing in */
void renameworld(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg.empty() || arg.length() > 24)
    {
        on::ConsoleMessage(event.peer, "`4Usage: /renameworld {newname}  (max 24 characters)``");
        return;
    }

    for (char &c : arg) c = std::toupper(static_cast<unsigned char>(c));

    for (char c : arg)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)))
        {
            on::ConsoleMessage(event.peer, "`4Letters and numbers only.``");
            return;
        }
    }

    if (std::ranges::find(worlds, arg, &::world::name) != worlds.end())
    {
        on::ConsoleMessage(event.peer, "`4A world with that name is already loaded.``");
        return;
    }

    const std::string old_name = world->name;

    // rename the row, then the in-memory copy
    {
        ::hStmt hStmt{ "UPDATE world SET name = ? WHERE name = ?" };
        MYSQL_BIND params[2] = { make_bind_in(arg), make_bind_in(old_name) };
        hStmt.bind_param(params);
        hStmt.execute();
    }

    world->name = arg;

    // everyone standing here needs their world reference updated
    peers(old_name, PEER_SAME_WORLD, [&arg, &old_name](ENetPeer &p)
    {
        ::peer *pOthers = static_cast<::peer*>(p.data);
        if (!pOthers) return;
        if (pOthers->recent_worlds.back() == old_name) pOthers->recent_worlds.back() = arg;
        on::ConsoleMessage(&p, std::format("`5This world is now called `w{}``.``", arg));
    });

    printf("[renameworld] %s -> %s\n", old_name.c_str(), arg.c_str());
}
