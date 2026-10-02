#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "friendsys.hpp"

namespace
{
std::vector<std::pair<int, int>> pending{}; // @note friend requests {from, to}

std::string clean(std::string s)
{
    for (char &c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

std::vector<int> friends_of(int uid)
{
    std::vector<int> out{};
    if (mysql_query(db, std::format("SELECT friend_uid FROM friends WHERE uid = {}", uid).c_str())) return out;
    if (MYSQL_RES *res = mysql_store_result(db))
    {
        while (MYSQL_ROW r = mysql_fetch_row(res)) if (r[0]) out.push_back(std::atoi(r[0]));
        mysql_free_result(res);
    }
    return out;
}

bool are_friends(int a, int b) { const std::vector<int> list = friends_of(a); return std::ranges::find(list, b) != list.end(); }

void link(int a, int b) { mysql_query(db, std::format("INSERT IGNORE INTO friends (uid, friend_uid) VALUES ({0},{1}),({1},{0})", a, b).c_str()); }
void unlink(int a, int b) { mysql_query(db, std::format("DELETE FROM friends WHERE (uid = {0} AND friend_uid = {1}) OR (uid = {1} AND friend_uid = {0})", a, b).c_str()); }

std::string name_of(int uid)
{
    std::string name{};
    if (mysql_query(db, std::format("SELECT growid FROM peer WHERE uid = {}", uid).c_str())) return "?";
    if (MYSQL_RES *res = mysql_store_result(db))
    {
        if (MYSQL_ROW r = mysql_fetch_row(res)) if (r[0]) name = r[0];
        mysql_free_result(res);
    }
    return name.empty() ? std::format("user #{}", uid) : name;
}

int role_of(int uid)
{
    int role = 0;
    if (mysql_query(db, std::format("SELECT role FROM peer WHERE uid = {}", uid).c_str())) return 0;
    if (MYSQL_RES *res = mysql_store_result(db))
    {
        if (MYSQL_ROW r = mysql_fetch_row(res)) if (r[0]) role = std::atoi(r[0]);
        mysql_free_result(res);
    }
    return role;
}

std::string colored(const std::string &name, int role)
{
    if (role >= DEVELOPER) return std::format("`6@{}``", name);
    if (role >= MODERATOR) return std::format("`5@{}``", name);
    return std::format("`w{}``", name);
}

ENetPeer *online(int uid)
{
    ENetPeer *hit = nullptr;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (t && t->user_id == uid && !t->growid.empty()) hit = &p;
    });
    return hit;
}
}

void friend_request(ENetEvent &event, ENetPeer &target)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);
    ::peer *t = static_cast<::peer*>(target.data);

    if (me == t) { tell(event.peer, "`4You can't add yourself.``"); return; }
    if (t->role >= MODERATOR && me->role < MODERATOR) { tell(event.peer, "`4You can't add staff as a friend.``"); return; }
    if (are_friends(me->user_id, t->user_id)) { tell(event.peer, std::format("`w{}`` is already your friend.", t->growid)); return; }

    // they already asked me: that's a yes
    if (auto r = std::ranges::find(pending, std::make_pair(t->user_id, me->user_id)); r != pending.end())
    {
        pending.erase(r);
        link(me->user_id, t->user_id);
        on::ConsoleMessage(event.peer, std::format("`2You and `w{}`` are friends now!``", t->growid));
        on::ConsoleMessage(&target, std::format("`2You and `w{}`` are friends now!``", me->growid));
        return;
    }
    if (std::ranges::find(pending, std::make_pair(me->user_id, t->user_id)) != pending.end()) { tell(event.peer, "`4You already sent them a request.``"); return; }

    pending.emplace_back(me->user_id, t->user_id);
    on::ConsoleMessage(event.peer, std::format("`2Friend request sent to `w{}``.``", t->growid));
    on::ConsoleMessage(&target, std::format("`5{} sent you a friend request!``", me->growid));
    send_varlist(&target, { "OnDialogRequest", std::format(
        "set_default_color|`o\n"
        "add_label_with_icon|big|`wFriend request``|left|1366|\n"
        "add_textbox|`w{}`` wants to be your friend.|left|\n"
        "embed_data|from|{}\n"
        "add_button|fr_yes|`2Accept``|noflags|0|0|\n"
        "add_button|fr_no|`4Decline``|noflags|0|0|\n"
        "end_dialog|friend_request|||\n", clean(me->growid), me->user_id) });
}

void friends_show(ENetEvent &event, bool edit)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);
    const std::vector<int> list = friends_of(me->user_id);

    std::string on_lines{}, off_lines{};
    int on_count = 0;
    for (int uid : list)
    {
        ENetPeer *p = online(uid);
        const ::peer *t = p ? static_cast<::peer*>(p->data) : nullptr;
        const std::string name = clean(t ? t->growid : name_of(uid));
        const int role = t ? t->role : role_of(uid);
        const std::string shown = colored(name, role); // @note dev gold, mod pink, player white - same as in game
        std::string &lines = p ? on_lines : off_lines;
        if (t)
        {
            ++on_count;
            lines += std::format("add_label_with_icon|small|{} `o- in `w{}``|left|1366|\n", shown, t->netid != 0 ? clean(t->recent_worlds.back()) : "the world menu");
        }
        else lines += std::format("add_label|small|{} `o- `4offline``|left|\n", shown);
        if (edit) lines += std::format("add_button|fr_rm_{}|Remove {}|noflags|0|0|\n", uid, name);
    }

    std::string d = "set_default_color|`o\n";
    d += std::format("add_label_with_icon|big|{} of {} `wFriends Online``|left|1366|\nadd_spacer|small|\n", on_count, list.size());
    if (list.empty()) d += "add_textbox|`oYou don't have any friends yet. Wrench a player and choose `wAdd as friend``.``|left|\n";
    d += on_lines + off_lines + "add_spacer|small|\n";
    d += edit ? "add_button|fr_done|Done editing|noflags|0|0|\n" : "add_button|fr_edit|Edit Friends|noflags|0|0|\n";
    d += "add_quick_exit|\nend_dialog|friends_list|Close||\n";
    send_varlist(event.peer, { "OnDialogRequest", d });
}

void friends_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);
    const std::string dialog = hPipe["dialog_name"], btn = hPipe["buttonClicked"];

    if (dialog == "friend_request")
    {
        const int from = std::atoi(hPipe["from"].c_str());
        auto r = std::ranges::find(pending, std::make_pair(from, me->user_id));
        if (r == pending.end()) { tell(event.peer, "`4That friend request isn't valid anymore.``"); return; }
        pending.erase(r);
        ENetPeer *asker = online(from);
        if (btn != "fr_yes")
        {
            if (asker) on::ConsoleMessage(asker, std::format("`4{} declined your friend request.``", me->growid));
            return;
        }
        link(from, me->user_id);
        on::ConsoleMessage(event.peer, std::format("`2You and `w{}`` are friends now!``", name_of(from)));
        if (asker) on::ConsoleMessage(asker, std::format("`2You and `w{}`` are friends now!``", me->growid));
        return;
    }

    if (btn == "fr_edit") friends_show(event, true);
    else if (btn == "fr_done") friends_show(event, false);
    else if (btn.starts_with("fr_rm_"))
    {
        const int uid = std::atoi(btn.c_str() + 6);
        const std::string name = name_of(uid);
        unlink(me->user_id, uid);
        on::ConsoleMessage(event.peer, std::format("`oRemoved `w{}`` from your friends.``", name));
        friends_show(event, true);
    }
}

std::string friends_welcome(const ::peer &p)
{
    int n = 0;
    for (int uid : friends_of(p.user_id)) if (online(uid)) ++n;
    return n == 0 ? "No friends are online." : std::format("`w{}`` friend{} online.", n, n == 1 ? " is" : "s are");
}

void friends_alert(const ::peer &p, bool logged_on)
{
    for (int uid : friends_of(p.user_id))
        if (ENetPeer *f = online(uid); f && f->data != nullptr && static_cast<::peer*>(f->data) != &p)
            on::ConsoleMessage(f, std::format("`3FRIEND ALERT:`` `w{}`` has {}!", p.growid, logged_on ? "`2logged on``" : "`4logged off``"));
}
