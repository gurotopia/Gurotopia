#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "legendary.hpp"
#include "lockaccess.hpp"

static std::string base_name(const ::peer &p, const ::world *w)
{
    if (p.role >= DEVELOPER) return std::format("`6@{}``", p.growid);
    if (p.role >= MODERATOR) return std::format("`5@{}``", p.growid);
    if (w && w->owner != 0 && p.user_id == w->owner) return std::format("`2{}``", p.growid);
    if (w && p.user_id != 0 && std::ranges::find(w->access, p.user_id) != w->access.end()) return std::format("`^{}``", p.growid); // @note light green, same as mod chat
    return std::format("`w{}``", p.growid);
}

std::string display_name_for(const ::peer &p, const ::world &w) { return title_name(p, p.nickname.empty() ? base_name(p, &w) : std::format("`w{}``", p.nickname)); } // @note a /nick keeps its titles

std::string chat_name(const ::peer &p)
{
    if (!p.nickname.empty()) return p.display_growid; // @note disguised with /nick
    const ::world *here = nullptr;
    if (p.netid != 0)
        if (auto w = std::ranges::find(worlds, p.recent_worlds.back(), &::world::name); w != worlds.end()) here = &*w;
    return title_name(p, base_name(p, here));
}

void refresh_display_names(::world &w)
{
    peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || !t->nickname.empty()) return;
        const std::string name = display_name_for(*t, w);
        if (name == t->display_growid) return;
        t->display_growid = name;
        peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &o) { send_varlist(&o, { "OnNameChanged", name }, t->netid); });
    });
}

/* GrowID for an account number: online players first, else the database */
std::string access_name_of(int uid)
{
    std::string name{};
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (t && t->user_id == uid) name = t->growid;
    });
    if (!name.empty()) return name;

    ::hStmt hStmt{ "SELECT growid FROM peer WHERE uid = ? LIMIT 1" };
    MYSQL_BIND param = make_bind_in(uid);
    hStmt.bind_param(&param);
    u_long length = 0;
    MYSQL_BIND result = make_bind_out(name);
    result.length = &length;
    mysql_stmt_bind_result(hStmt.pStmt, &result);
    hStmt.execute();
    hStmt.fetch();
    name.resize(length);
    return name.empty() ? std::format("user #{}", uid) : name;
}

bool lock_access_add(ENetEvent &event, ::world &w, int uid)
{
    if (uid == 0) return false;
    if (uid == w.owner) { tell(event.peer, "`4They already own this world.``"); return false; }
    const std::string name = access_name_of(uid);
    if (std::ranges::find(w.access, uid) != w.access.end()) { tell(event.peer, std::format("`w{}`` already has access.", name)); return false; }
    auto slot = std::ranges::find(w.access, 0);
    if (slot == w.access.end()) { tell(event.peer, "`4The access list is full (20 people).``"); return false; }

    *slot = uid;
    refresh_display_names(w); // @note their name turns light green for everyone right now

    on::ConsoleMessage(event.peer, std::format("`2`w{}`` now has access to `w{}``.``", name, w.name));
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (t && t->user_id == uid && &p != event.peer) on::ConsoleMessage(&p, std::format("`2You've been given access to `w{}``!`` You can build and break there now.", w.name));
    });
    return true;
}

bool lock_access_remove(ENetEvent &event, ::world &w, int uid)
{
    auto slot = std::ranges::find(w.access, uid);
    if (uid == 0 || slot == w.access.end()) return false;
    *slot = 0;
    refresh_display_names(w);
    on::ConsoleMessage(event.peer, std::format("`oRemoved `w{}``'s access to `w{}``.``", access_name_of(uid), w.name));
    return true;
}

std::string lock_access_rows(const ::world &w)
{
    std::string rows{};
    for (int uid : w.access)
        if (uid != 0) rows += std::format("add_checkbox|checkbox_{}|{}|1\n", uid, access_name_of(uid));
    if (rows.empty()) return "add_label|small|Currently, you're the only one with access.``|left\n";
    return "add_label|small|`wPlayers with access`` (uncheck a name to remove it):``|left\n" + rows;
}

void lock_access_apply(ENetEvent &event, ::world &w, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (w.owner != pPeer->user_id && pPeer->role < DEVELOPER) return;

    /* unchecked names lose access */
    const std::array<int, 20> before = w.access;
    for (int uid : before)
        if (uid != 0 && hPipe[std::format("checkbox_{}", uid)] == "0") lock_access_remove(event, w, uid);

    /* "Add" picked a player */
    const std::string pick = hPipe["playerNetID"];
    if (pick.empty() || pick == "0") return;
    const int netid = std::atoi(pick.c_str());

    ::peer *target = nullptr;
    peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *o = static_cast<::peer*>(p.data);
        if (o && o->netid == netid) target = o;
    });
    if (!target) { tell(event.peer, "`4That player isn't in this world anymore.``"); return; }
    lock_access_add(event, w, target->user_id);
}
