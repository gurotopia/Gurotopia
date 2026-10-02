#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "commands/__command.hpp"
#include "commands/curse.hpp"
#include "staff.hpp"

static std::string rank_name(int role) { return role >= DEVELOPER ? "Developer" : role >= MODERATOR ? "Moderator" : "Player"; }

static std::string clean(std::string s)
{
    for (char &c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

/* "30" = minutes, or 45s, 30m, 2h, 7d, 1w. @return seconds, -1 if invalid */
static long long duration(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.empty()) return -1;
    long long mult = 60;
    const char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(s.back())));
    if (std::isalpha(static_cast<unsigned char>(unit)))
    {
        switch (unit) { case 's': mult = 1; break; case 'm': mult = 60; break; case 'h': mult = 3600; break;
                        case 'd': mult = 86400; break; case 'w': mult = 604800; break; default: return -1; }
        s.pop_back();
    }
    if (s.empty() || s.size() > 6 || !std::ranges::all_of(s, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })) return -1;
    return std::stoll(s) * mult;
}

static ENetPeer *by_netid(const std::string &world, int netid)
{
    ENetPeer *hit = nullptr;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p) { const ::peer *t = static_cast<::peer*>(p.data); if (t && t->netid == netid) hit = &p; });
    return hit;
}

static void run(ENetEvent &event, const std::string &text)
{
    const std::string name = text.substr(0, text.find(' '));
    if (auto it = cmd_pool.find(name); it != cmd_pool.end()) it->second(event, text);
    else tell(event.peer, "`4That command doesn't exist.``");
}

std::string staff_wrench_rows(const ::peer &viewer, const ::peer &t, ENetPeer &tp)
{
    const long long days = t.created_at > 0 ? (static_cast<long long>(std::time(nullptr)) - static_cast<long long>(t.created_at)) / 86400 : 0;
    std::string s = "add_spacer|small|\nadd_label|big|`wStaff``|left|\n";
    s += std::format("add_smalltext|Account `w#{}``  -  {}  -  created `w{}`` days ago|left|\n", t.user_id, rank_name(t.role), days);
    s += std::format("add_smalltext|Level `w{}``  -  Gems `w{}``  -  Tile `w{}``, `w{}``|left|\n", t.level.front(), t.gems, t.pos.by_32(true).x_int(), t.pos.by_32(true).y_int());
    if (viewer.role >= DEVELOPER)
    {
        char ip[64]{};
        enet_address_get_host_ip(&tp.address, ip, sizeof(ip));
        s += std::format("add_smalltext|IP `w{}``|left|\n", ip);
    }
    std::string status{};
    if (t.curse_until > std::time(nullptr)) status += std::format("`4Cursed`` ({})  ", curse_time_left(t));
    if (t.frozen || (t.state & S_FROZEN)) status += "`1Frozen``  ";
    if (t.state & S_DUCT_TAPE) status += "`oDuct taped``  ";
    if (t.god_mode) status += "`2God mode``  ";
    s += std::format("add_smalltext|Status: {}|left|\n", status.empty() ? "normal" : status);

    if (t.role >= viewer.role) return s + "add_smalltext|`oYou can't punish someone of equal or higher rank.``|left|\n";

    /* one step: fill in the time and reason, then press Curse or Ban */
    s += "add_text_input|staff_time|Time (30m, 2h, 1d, 1w):|30m|6|\n"
         "add_text_input|staff_reason|Reason:||60|\n"
         "add_button|staff_curse|`4Curse``|noflags|0|0|\n"
         "add_button|staff_ban|`4Ban``|noflags|0|0|\n";
    if (t.curse_until > std::time(nullptr)) s += "add_button|staff_uncurse|Lift curse|noflags|0|0|\n";
    s += "add_button|staff_kick|Kick from world|noflags|0|0|\n"
         "add_button|staff_freeze|Freeze / unfreeze|noflags|0|0|\n"
         "add_button|staff_ducttape|Duct tape / remove|noflags|0|0|\n"
         "add_button|staff_summon|Summon to me|noflags|0|0|\n"
         "add_button|staff_warpto|Warp to their world|noflags|0|0|\n";
    return s;
}

bool staff_popup(ENetEvent &event, const ::hPipe &hPipe)
{
    const std::string btn = hPipe["buttonClicked"];
    if (!btn.starts_with("staff_")) return false;
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) return true;

    ENetPeer *tp = by_netid(pPeer->recent_worlds.back(), std::atoi(hPipe["netID"].c_str()));
    if (!tp) { tell(event.peer, "`4That player isn't here anymore.``"); return true; }
    ::peer *t = static_cast<::peer*>(tp->data);
    const std::string name = t->growid;
    if (btn == "staff_panel") // @note the separate staff window; its buttons come back here through "popup"
    {
        send_varlist(event.peer, { "OnDialogRequest", std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wStaff: {}``|left|732|\n"
            "embed_data|netID|{}\n"
            "{}"
            "add_spacer|small|\n"
            "end_dialog|popup|Close||\n", t->display_growid, t->netid, staff_wrench_rows(*pPeer, *t, *tp)) });
        return true;
    }
    if (t->role >= pPeer->role) { tell(event.peer, "`4You can't do that to someone of equal or higher rank.``"); return true; }

    std::string time = hPipe["staff_time"], reason = clean(hPipe["staff_reason"]);
    while (!time.empty() && std::isspace(static_cast<unsigned char>(time.back()))) time.pop_back();
    if (time.empty()) time = "30m";
    if (reason.empty()) reason = "No reason given";

    if (btn == "staff_curse")
    {
        printf("[staff] %s curses %s for %s: %s\n", pPeer->growid.c_str(), name.c_str(), time.c_str(), reason.c_str());
        run(event, std::format("curse {} {} {}", name, time, reason));
    }
    else if (btn == "staff_ban")
    {
        const long long secs = duration(time);
        if (secs <= 0) { tell(event.peer, "`4Invalid time.`` Use e.g. 30m, 2h, 1d or 1w."); return true; }
        t->ban_until = std::time(nullptr) + std::min(secs, 315360000LL);
        t->mysql_update("ban_until", static_cast<signed>(t->ban_until));
        peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
        {
            const ::peer *o = static_cast<::peer*>(p.data);
            if (o && !o->growid.empty()) on::ConsoleMessage(&p, std::format("`#** `$The Ancient Ones`` have `4banned`` `w{}``! `#**``", name));
        });
        on::ConsoleMessage(tp, std::format("`4You have been banned`` for `w{}``. Reason: `w{}``", time, reason));
        on::ConsoleMessage(event.peer, std::format("`2Banned `w{}`` for {}.``", name, time));
        printf("[staff] %s banned %s for %s: %s\n", pPeer->growid.c_str(), name.c_str(), time.c_str(), reason.c_str());
        enet_peer_disconnect_later(tp, 0);
    }
    else if (btn == "staff_uncurse")  run(event, "uncurse " + name);
    else if (btn == "staff_kick")     run(event, "kick " + name);
    else if (btn == "staff_freeze")   run(event, "freeze " + name);
    else if (btn == "staff_ducttape") run(event, "ducttape " + name);
    else if (btn == "staff_summon")   run(event, "summon " + name);
    else if (btn == "staff_warpto")   run(event, "warpto " + name);
    return true;
}
