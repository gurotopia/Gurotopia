#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/NameChanged.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "lockaccess.hpp"
#include "setowner.hpp"

/* /setowner {player}  -> hands this world to that account
 * /setowner           -> clears ownership
 */
void setowner(ENetEvent& event, const std::string_view text)
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

    if (arg.empty())
    {
        world->owner = 0;
        refresh_display_names(*world);
        on::ConsoleMessage(event.peer, std::format("`2`w{}`` has no owner now.``", world->name));
        return;
    }

    std::string lower_name = arg;
    for (char &c : lower_name) c = std::tolower(static_cast<unsigned char>(c));

    // look the account up by name
    signed uid = 0;
    {
        ::hStmt hStmt{ "SELECT uid FROM peer WHERE growid = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(lower_name);
        hStmt.bind_param(&param);

        MYSQL_BIND result = make_bind_out(uid);
        mysql_stmt_bind_result(hStmt.pStmt, &result);
        hStmt.execute();
        hStmt.fetch();
    }

    if (uid == 0)
    {
        on::ConsoleMessage(event.peer, std::format("`4No account named `w{}`` exists.``", arg));
        return;
    }

    world->owner = uid;
    refresh_display_names(*world);

    on::ConsoleMessage(event.peer,
        std::format("`2`w{}`` now belongs to `w{}``.``", world->name, arg));
}
