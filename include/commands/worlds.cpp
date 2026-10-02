#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "worlds.hpp"

/* /worlds -> lists every world stored in the database */
void worlds_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    ::hStmt hStmt{ "SELECT name FROM world ORDER BY name" };
    hStmt.execute();

    char buffer[32]{};
    u_long length = 0;
    MYSQL_BIND result{};
    result.buffer_type = MYSQL_TYPE_STRING;
    result.buffer = buffer;
    result.buffer_length = sizeof(buffer);
    result.length = &length;
    mysql_stmt_bind_result(hStmt.pStmt, &result);

    std::vector<std::string> names;
    while (mysql_stmt_fetch(hStmt.pStmt) == 0)
    {
        names.emplace_back(buffer, length);
        memset(buffer, 0, sizeof(buffer));
    }

    if (names.empty())
    {
        on::ConsoleMessage(event.peer, "`4No worlds stored yet.``");
        return;
    }

    // which of them are loaded right now
    std::string line{};
    for (const std::string &n : names)
    {
        const bool loaded = std::ranges::find(worlds, n, &::world::name) != worlds.end();
        if (!line.empty()) line += "`o, ";
        line += loaded ? std::format("`2{}``", n) : std::format("`w{}``", n);
    }

    on::ConsoleMessage(event.peer,
        std::format("`o{} world{}: {}", names.size(), names.size() == 1 ? "" : "s", line));
    on::ConsoleMessage(event.peer, "`o(`2green`o = currently loaded)");
}
