#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "unban.hpp"

/* /unban {player} -> lifts a ban (wrench Ban, Ban Wand) right away */
void unban_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();
    if (arg.empty() || arg.size() > 18 || !std::ranges::all_of(arg, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0; }))
    {
        on::ConsoleMessage(event.peer, "`4Usage: /unban {player}``");
        return;
    }

    // look the account up (names are letters and numbers only, checked above)
    int uid = 0, until = 0;
    std::string name{};
    if (mysql_query(db, std::format("SELECT uid, ban_until, growid FROM peer WHERE growid = '{}' LIMIT 1", arg).c_str()) == 0)
        if (MYSQL_RES *res = mysql_store_result(db))
        {
            if (MYSQL_ROW r = mysql_fetch_row(res))
            {
                uid = r[0] ? std::atoi(r[0]) : 0;
                until = r[1] ? std::atoi(r[1]) : 0;
                name = r[2] ? r[2] : arg;
            }
            mysql_free_result(res);
        }
    if (uid == 0)
    {
        on::ConsoleMessage(event.peer, std::format("`4No account named `w{}`` exists.``", arg));
        return;
    }
    if (until <= std::time(nullptr))
    {
        on::ConsoleMessage(event.peer, std::format("`w{}`` isn't banned.", name));
        return;
    }

    mysql_query(db, std::format("UPDATE peer SET ban_until = 0 WHERE uid = {}", uid).c_str());
    printf("[staff] %s unbanned %s\n", pPeer->growid.c_str(), name.c_str());
    on::ConsoleMessage(event.peer, std::format("`2Unbanned `w{}``. They can log in again.``", name));
}
