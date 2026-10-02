#include "pch.hpp"
#include <ctime>
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "tools/create_dialog.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "accounts.hpp"

namespace
{
using row_t = std::vector<std::pair<std::string, std::string>>; // @note {column, value}, binary columns left out

/* ------------------------------------------------------------------ database */
std::vector<row_t> query(const std::string &sql)
{
    std::vector<row_t> out{};
    if (mysql_query(db, sql.c_str())) { std::fprintf(stderr, "[accounts] %s\n", mysql_error(db)); return out; }
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) return out;
    const unsigned n = mysql_num_fields(res);
    MYSQL_FIELD *f = mysql_fetch_fields(res);
    while (MYSQL_ROW r = mysql_fetch_row(res))
    {
        unsigned long *len = mysql_fetch_lengths(res);
        row_t row{};
        for (unsigned i = 0; i < n; ++i)
        {
            const bool blob = (f[i].type == MYSQL_TYPE_BLOB || f[i].type == MYSQL_TYPE_TINY_BLOB ||
                               f[i].type == MYSQL_TYPE_MEDIUM_BLOB || f[i].type == MYSQL_TYPE_LONG_BLOB) && f[i].charsetnr == 63;
            if (blob) continue;
            row.emplace_back(f[i].name, r[i] ? std::string(r[i], len[i]) : std::string{});
        }
        out.push_back(std::move(row));
    }
    mysql_free_result(res);
    return out;
}

bool run(const std::string &sql)
{
    if (mysql_query(db, sql.c_str())) { std::fprintf(stderr, "[accounts] %s\n", mysql_error(db)); return false; }
    return true;
}

bool has(const row_t &r, std::string_view name) { for (const auto &[k, v] : r) if (k == name) return true; return false; }
std::string col(const row_t &r, std::string_view name) { for (const auto &[k, v] : r) if (k == name) return v; return {}; }

std::string esc(const std::string &s)
{
    std::string out(s.size() * 2 + 1, '\0');
    out.resize(mysql_real_escape_string(db, out.data(), s.c_str(), static_cast<unsigned long>(s.size())));
    return out;
}

std::string clean(std::string s)
{
    for (char &c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

/* ------------------------------------------------------------------ formatting */
std::string number(long long v)
{
    std::string s = std::to_string(v < 0 ? -v : v);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<std::size_t>(i), ",");
    return (v < 0 ? "-" : "") + s;
}

std::string date(std::time_t t)
{
    char buf[40]{};
    std::strftime(buf, sizeof(buf), "%d %b %Y, %H:%M", std::localtime(&t));
    return buf;
}

std::string left(long long secs)
{
    const long long d = secs / 86400, h = (secs % 86400) / 3600, m = (secs % 3600) / 60;
    if (d > 0) return std::format("{}d {}h left", d, h);
    if (h > 0) return std::format("{}h {}m left", h, m);
    return std::format("{}m left", std::max(1LL, m));
}

/* "2026-09-20 14:32:05" -> "20 Sep 2026, 14:32" */
std::string sql_date(const std::string &s)
{
    static const char *months[]{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    if (s.size() < 16) return s.empty() ? "unknown" : s;
    const int mo = std::atoi(s.substr(5, 2).c_str());
    return std::format("{} {} {}, {}", s.substr(8, 2), (mo >= 1 && mo <= 12) ? months[mo - 1] : "?", s.substr(0, 4), s.substr(11, 5));
}

std::string until(const std::string &v)
{
    const long long t = std::atoll(v.c_str()), now = static_cast<long long>(std::time(nullptr));
    if (t <= now) return "No";
    return std::format("`4Yes``, until {} ({})", date(static_cast<std::time_t>(t)), left(t - now));
}

std::string rank_name(int role) { return role >= DEVELOPER ? "Developer" : role >= MODERATOR ? "Moderator" : "Player"; }
std::string rank_color(int role) { return role >= DEVELOPER ? "`6" : role >= MODERATOR ? "`#" : "`w"; }

std::string pretty(std::string k)
{
    for (char &c : k) if (c == '_') c = ' ';
    if (!k.empty()) k[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(k[0])));
    return k;
}

/* ------------------------------------------------------------------ online */
const ::peer *online(int uid)
{
    const ::peer *hit = nullptr;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (t && t->user_id == uid) hit = t;
    });
    return hit;
}

std::string online_text(int uid)
{
    const ::peer *t = online(uid);
    if (!t) return "`sOffline``";
    if (t->netid != 0 && !t->recent_worlds.empty() && !t->recent_worlds.back().empty())
        return std::format("`2Online`` in `w{}``", t->recent_worlds.back());
    return "`2Online`` (choosing a world)";
}

/* ------------------------------------------------------------------ pages */
void send(ENetEvent &event, ::create_dialog &d)
{
    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("accounts", "Close", "") });
}

void line(::create_dialog &d, const std::string &label, const std::string &value, int icon)
{
    d.add_label_with_icon("small", std::format("{}: `w{}``", label, value), icon);
}

void show_list(ENetEvent &event, const std::string &search)
{
    const std::string where = search.empty() ? "" : " WHERE growid LIKE '%" + esc(search) + "%'";
    const std::vector<row_t> rows = query("SELECT * FROM peer" + where + " ORDER BY uid DESC LIMIT 100");
    const std::vector<row_t> total = query("SELECT COUNT(*) AS n FROM peer");
    int online_now = 0;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p) { if (p.data && !static_cast<::peer*>(p.data)->growid.empty()) ++online_now; });

    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", "`wAccounts``", 1280)
     .add_smalltext(std::format("`w{}`` accounts  -  `2{}`` online now", total.empty() ? "?" : number(std::atoll(col(total[0], "n").c_str())), online_now))
     .add_spacer("small")
     .add_text_input("search", "Find a player:", clean(search), 18)
     .add_button("acc_search", "Search");
    if (!search.empty()) d.add_button("acc_back", "Show all");
    d.add_spacer("small");

    if (rows.empty()) d.add_textbox("No accounts found.");
    else d.add_smalltext(search.empty() ? "Newest first:" : std::format("Matching `w{}``:", clean(search)));

    for (const row_t &r : rows)
    {
        const int uid = std::atoi(col(r, "uid").c_str());
        const int role = std::atoi(col(r, "role").c_str());
        std::string info = std::format("#{}", uid);
        if (has(r, "level")) info += std::format("  -  Level {}", col(r, "level"));
        info += std::format("  -  Created {}", sql_date(col(r, "created_at")));
        if (online(uid)) info += "  -  `2online``";

        d.add_spacer("small")
         .add_button(std::format("acc_{}", uid), std::format("{}{}``", rank_color(role), clean(col(r, "growid"))))
         .add_smalltext(info);
    }
    send(event, d);
}

void show_details(ENetEvent &event, int uid)
{
    const std::vector<row_t> rows = query(std::format("SELECT * FROM peer WHERE uid = {}", uid));
    if (rows.empty()) { tell(event.peer, "`4That account doesn't exist anymore.``"); show_list(event, ""); return; }
    const row_t &r = rows[0];
    const int role = std::atoi(col(r, "role").c_str());
    const std::vector<row_t> owned = query(std::format("SELECT name FROM world WHERE owner = {} ORDER BY name", uid));

    static const std::vector<std::string> shown{ "uid", "growid", "password", "created_at", "role", "renamed_at",
                                                 "level", "xp", "gems", "curse_until", "ban_until", "god" };

    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", std::format("{}{}``", rank_color(role), clean(col(r, "growid"))), 1280)
     .add_smalltext(std::format("{}  -  {}", rank_name(role), online_text(uid)));

    d.add_spacer("small").add_label("big", "`wAccount``");
    line(d, "ID", std::format("#{}", uid), 1280);
    line(d, "Created", sql_date(col(r, "created_at")), 1280);
    line(d, "Rank", rank_name(role), role >= MODERATOR ? 278 : 18);
    if (has(r, "renamed_at"))
    {
        const long long t = std::atoll(col(r, "renamed_at").c_str());
        line(d, "Last name change", t > 0 ? date(static_cast<std::time_t>(t)) : "Never", 1280);
    }

    if (has(r, "level") || has(r, "xp") || has(r, "gems"))
    {
        d.add_spacer("small").add_label("big", "`wProgress``");
        if (has(r, "level")) line(d, "Level", col(r, "level"), 1488);
        if (has(r, "xp"))    line(d, "XP", number(std::atoll(col(r, "xp").c_str())), 1488);
        if (has(r, "gems"))  line(d, "Gems", number(std::atoll(col(r, "gems").c_str())), 112);
    }

    d.add_spacer("small").add_label("big", "`wStatus``");
    if (has(r, "curse_until")) line(d, "Cursed", until(col(r, "curse_until")), 278);
    if (has(r, "ban_until"))   line(d, "Banned", until(col(r, "ban_until")), 732);
    if (has(r, "god"))         line(d, "God mode", std::atoi(col(r, "god").c_str()) ? "`2On``" : "Off", 128);

    d.add_spacer("small").add_label("big", std::format("`wWorlds ({})``", owned.size()));
    if (owned.empty()) d.add_smalltext("Doesn't own any worlds.");
    for (const row_t &w : owned) d.add_label_with_icon("small", std::format("`w{}``", clean(col(w, "name"))), 242);

    bool other = false;
    for (const auto &[k, v] : r)
    {
        if (std::ranges::find(shown, k) != shown.end()) continue;
        if (!other) { d.add_spacer("small").add_label("big", "`wOther``"); other = true; }
        d.add_smalltext(std::format("{}: `w{}``", pretty(k), v.empty() ? "-" : clean(v)));
    }

    d.embed_data("uid", uid)
     .add_spacer("small")
     .add_button("acc_delete", "`4Delete Account``")
     .add_button("acc_back", "Back to list");
    send(event, d);
}

void confirm_delete(ENetEvent &event, int uid)
{
    const std::vector<row_t> rows = query(std::format("SELECT growid FROM peer WHERE uid = {}", uid));
    if (rows.empty()) return;
    const std::vector<row_t> owned = query(std::format("SELECT name FROM world WHERE owner = {}", uid));

    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", std::format("`4Delete {}?``", clean(col(rows[0], "growid"))), 1280)
     .add_textbox("`4This can't be undone.`` The account, its backpack and its progress will be gone for good.")
     .add_smalltext(owned.empty() ? "They don't own any worlds." : std::format("They own `w{}`` world(s). What should happen to them?", owned.size()))
     .embed_data("uid", uid)
     .add_spacer("small")
     .add_button("acc_del_keep", owned.empty() ? "`4Delete Account``" : "Delete account, keep the worlds (unlocked)");
    if (!owned.empty()) d.add_button("acc_del_worlds", "`4Delete account AND their worlds``");
    d.add_button(std::format("acc_{}", uid), "Cancel");
    send(event, d);
}

void do_delete(ENetEvent &event, int uid, bool with_worlds)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (uid == pPeer->user_id) { tell(event.peer, "`4You can't delete your own account.``"); return; }
    if (online(uid)) { tell(event.peer, "`4That player is online. Kick them first.``"); return; }

    const std::vector<row_t> rows = query(std::format("SELECT growid FROM peer WHERE uid = {}", uid));
    if (rows.empty()) return;
    const std::string growid = col(rows[0], "growid");

    int deleted = 0, unlocked = 0;
    for (const row_t &w : query(std::format("SELECT name FROM world WHERE owner = {}", uid)))
    {
        const std::string name = col(w, "name");
        auto loaded = std::ranges::find(worlds, name, &::world::name);
        if (with_worlds && loaded == worlds.end())
        {
            if (run("DELETE FROM world WHERE name = '" + esc(name) + "'")) ++deleted;
        }
        else
        {
            if (loaded != worlds.end()) loaded->owner = 0;
            if (run("UPDATE world SET owner = 0 WHERE name = '" + esc(name) + "'")) ++unlocked;
        }
    }
    run("DELETE FROM renames WHERE new_name = '" + esc(growid) + "'");
    if (!run(std::format("DELETE FROM peer WHERE uid = {}", uid))) { tell(event.peer, "`4Deleting failed - see the server window.``"); return; }

    printf("[accounts] %s deleted account %s (#%d): %d worlds deleted, %d unlocked\n", pPeer->growid.c_str(), growid.c_str(), uid, deleted, unlocked);
    on::ConsoleMessage(event.peer, std::format("`2Deleted `w{}``. Worlds deleted: `w{}``, unlocked: `w{}``.``", clean(growid), deleted, unlocked));
    show_list(event, "");
}
}

/* /accounts [search] */
void accounts_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;
    std::string search{};
    if (const std::size_t sp = text.find(' '); sp != std::string_view::npos) search = std::string(text.substr(sp + 1));
    show_list(event, search);
}

void accounts_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;

    const std::string btn = hPipe["buttonClicked"];
    const int uid = std::atoi(hPipe["uid"].c_str());

    if (btn == "acc_search")          show_list(event, hPipe["search"]);
    else if (btn == "acc_back")       show_list(event, "");
    else if (btn == "acc_delete")     confirm_delete(event, uid);
    else if (btn == "acc_del_keep")   do_delete(event, uid, false);
    else if (btn == "acc_del_worlds") do_delete(event, uid, true);
    else if (btn.starts_with("acc_")) show_details(event, std::atoi(btn.substr(4).c_str()));
}
