#include "pch.hpp"
#include <map>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/CountryState.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "flag.hpp"

namespace
{
std::map<int, std::string> flags{}; // @note uid -> chosen flag, "" = default

void ensure_table()
{
    static bool done = false;
    if (done) return;
    done = true;
    mysql_query(db, "CREATE TABLE IF NOT EXISTS flags (uid INT PRIMARY KEY, country VARCHAR(16) NOT NULL)");
}

const std::string &chosen(int uid)
{
    if (auto it = flags.find(uid); it != flags.end()) return it->second;
    ensure_table();
    std::string c{};
    if (mysql_query(db, std::format("SELECT country FROM flags WHERE uid = {}", uid).c_str()) == 0)
        if (MYSQL_RES *res = mysql_store_result(db))
        {
            if (MYSQL_ROW r = mysql_fetch_row(res)) c = r[0] ? r[0] : "";
            mysql_free_result(res);
        }
    return flags[uid] = c;
}

/* badges and title icons: devs only (titles give the icons themselves); the icon sheets would look broken */
bool staff_flag(const std::string &code)
{
    return code == "ccbadge" || code == "ttbadge" || code == "g4g" || code == "master_icon" || code == "game_icons" || code == "faction_icons";
}
}

std::string flag_of(const ::peer &p)
{
    if (p.user_id != 0)
        if (const std::string &c = chosen(static_cast<int>(p.user_id)); !c.empty()) return c;
    if (p.role >= DEVELOPER) return "ccBadge"; // @note devs get the blue checkmark
    return p.country;
}

/* /flag           - show your flag
 * /flag {code}    - change it (no, us, jp ...)
 * /flag reset     - back to the default (devs: the checkmark badge) */
void flag_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->user_id == 0) { on::ConsoleMessage(event.peer, "`4You need a GrowID to change your flag.``"); return; }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();
    for (char &c : arg) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, std::format("`oYour flag is `w{}``. Use `w/flag {{code}}`` (like `wno``, `wus``, `wjp``), `w/flag none`` or `w/flag reset``.``", flag_of(*pPeer)));
        return;
    }
    if (arg == "reset") arg.clear();
    else if (arg.size() < 2 || arg.size() > 16 || !std::ranges::all_of(arg, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }))
    {
        on::ConsoleMessage(event.peer, "`4That's not a flag code.`` Use a country code like `wse``, `wno``, `wus``.");
        return;
    }
    else if (staff_flag(arg) && pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4That flag is for staff only.``");
        return;
    }

    const int uid = static_cast<int>(pPeer->user_id);
    (void)chosen(uid); // @note makes sure the table exists
    flags[uid] = arg;
    if (arg.empty()) mysql_query(db, std::format("DELETE FROM flags WHERE uid = {}", uid).c_str());
    else mysql_query(db, std::format("REPLACE INTO flags (uid, country) VALUES ({}, '{}')", uid, arg).c_str()); // @note arg is letters/digits only

    on::CountryState(event);
    if (pPeer->netid != 0)
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p)
        {
            ::peer *o = static_cast<::peer*>(p.data);
            if (!o || o->user_id == pPeer->user_id) return;
            on::CountryStateOf(p, *pPeer);
        });
    on::ConsoleMessage(event.peer, arg.empty() ? std::format("`2Flag reset to `w{}``.``", flag_of(*pPeer)) : std::format("`2Your flag is now `w{}``.``", arg));
}