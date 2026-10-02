#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/items.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "database/world.hpp"
#include "curse.hpp"

static std::string lower(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string take(std::string &rest)
{
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front()))) rest.erase(0, 1);
    const std::size_t sp = rest.find(' ');
    std::string token = rest.substr(0, sp);
    rest = (sp == std::string::npos) ? std::string{} : rest.substr(sp + 1);
    return token;
}

/* "30" = 30 minutes, or with a unit: 45s, 30m, 2h, 7d, 1w. @return seconds, -1 if invalid */
static long long parse_duration(std::string s)
{
    if (s.empty()) return -1;
    long long mult = 60;
    const char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(s.back())));
    if (std::isalpha(static_cast<unsigned char>(unit)))
    {
        switch (unit)
        {
            case 's': mult = 1; break;
            case 'm': mult = 60; break;
            case 'h': mult = 3600; break;
            case 'd': mult = 86400; break;
            case 'w': mult = 604800; break;
            default: return -1;
        }
        s.pop_back();
    }
    if (s.empty() || s.size() > 6) return -1;
    for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return -1;
    return std::stoll(s) * mult;
}

static ENetPeer *find_online(const std::string &name)
{
    const std::string want = lower(name);
    ENetPeer *found = nullptr;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (t && !t->growid.empty() && lower(t->growid) == want) found = &p;
    });
    return found;
}

std::string curse_time_left(const ::peer &p)
{
    long long left = static_cast<long long>(p.curse_until) - static_cast<long long>(std::time(nullptr));
    if (left < 0) left = 0;
    const long long d = left / 86400, h = left % 86400 / 3600, m = left % 3600 / 60, s = left % 60;
    if (d) return std::format("{} days, {} hours, {} mins, {} secs", d, h, m, s);
    if (h) return std::format("{} hours, {} mins, {} secs", h, m, s);
    if (m) return std::format("{} mins, {} secs", m, s);
    return std::format("{} secs", s);
}

/* /curse {player} {time} {reason} -> sends the player to HELL until the time runs out */
void curse_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        tell(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::string rest{ text };
    take(rest); // @note "curse"
    const std::string target = take(rest);
    const std::string length = take(rest);
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.back()))) rest.pop_back();
    const std::string reason = rest.empty() ? "No reason given" : rest;

    if (target.empty() || length.empty())
    {
        tell(event.peer, "`4Usage: /curse {player} {time} {reason}`` - time like 30m, 2h, 1d, 1w (plain number = minutes)");
        return;
    }

    long long seconds = parse_duration(length);
    if (seconds <= 0)
    {
        tell(event.peer, "`4Invalid time.`` Use e.g. 30m, 2h, 1d or 1w.");
        return;
    }
    seconds = std::min(seconds, 315360000LL); // @note cap at 10 years

    ENetPeer *found = find_online(target);
    if (!found)
    {
        tell(event.peer, std::format("`4`w{}`` is not online.``", target));
        return;
    }
    ::peer *pTarget = static_cast<::peer*>(found->data);

    if (pTarget != pPeer && pTarget->role >= pPeer->role)
    {
        tell(event.peer, "`4You cannot curse someone of equal or higher rank.``");
        return;
    }

    pTarget->curse_until = std::time(nullptr) + seconds;
    pTarget->mysql_update("curse_until", static_cast<signed>(pTarget->curse_until)); if (pTarget->netid != 0) on::SetClothing(*found);

    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *o = static_cast<::peer*>(p.data);
        if (!o || o->growid.empty()) return;
        on::ConsoleMessage(&p, std::format("`#** `$The Ancient Ones`` have used `#Curse`` on `w{}``! `#**``", pTarget->growid));
    });

    tell(found, std::format("`4You have been cursed`` for `w{}``. Reason: `w{}``", curse_time_left(*pTarget), reason));

    if (pTarget->netid != 0 && pTarget->recent_worlds.back() != "HELL")
    {
        ENetEvent ev{};
        ev.peer = found;
        action::quit_to_exit(ev, "", true);
        action::join_request(ev, "", "HELL");
        if (pTarget->netid == 0) // @note the join failed: say why instead of leaving them stuck
        {
            printf("[curse] could not move %s to HELL - see the [join] line above\n", pTarget->growid.c_str());
            on::ConsoleMessage(event.peer, "`4They are cursed, but moving them to HELL failed - see the server window for the reason.``");
        }
    }

    if (pTarget != pPeer)
        on::ConsoleMessage(event.peer, std::format("`2Cursed `w{}`` for {}.``", pTarget->growid, curse_time_left(*pTarget)));
}

/* /uncurse {player} -> lifts the curse early */
void uncurse_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        tell(event.peer, "`4Only staff can use this.``");
        return;
    }

    std::string rest{ text };
    take(rest); // @note "uncurse"
    const std::string target = take(rest);
    if (target.empty())
    {
        tell(event.peer, "`4Usage: /uncurse {player}``");
        return;
    }

    ENetPeer *found = find_online(target);
    if (!found)
    {
        tell(event.peer, std::format("`4`w{}`` is not online.``", target));
        return;
    }
    ::peer *pTarget = static_cast<::peer*>(found->data);

    if (pTarget->curse_until <= std::time(nullptr))
    {
        on::ConsoleMessage(event.peer, std::format("`w{}`` is not cursed.", pTarget->growid));
        return;
    }

    pTarget->curse_until = 0;
    pTarget->mysql_update("curse_until", static_cast<signed>(0)); if (pTarget->netid != 0) on::SetClothing(*found);

    if (pTarget->netid != 0 && pTarget->recent_worlds.back() == "HELL") // @note out of HELL, back to the world menu
    {
        ENetEvent ev{};
        ev.peer = found;
        action::quit_to_exit(ev, "", false);
    }
    on::ConsoleMessage(found, "`2Your curse has been lifted!``");
    if (pTarget != pPeer)
        on::ConsoleMessage(event.peer, std::format("`2Lifted the curse on `w{}``.``", pTarget->growid));
}


/* Curse Wand icon for the wrench menu, looked up by name */
short curse_icon()
{
    static short id = -1;
    if (id == -1)
    {
        id = 278;
        for (const ::item &it : items)
            if (it.raw_name == "Curse Wand") { id = static_cast<short>(it.id); break; }
    }
    return id;
}

/* "Curse (x left)" line for the wrench menu, empty when not cursed */
std::string curse_wrench_line(const ::peer &p)
{
    if (p.curse_until <= std::time(nullptr)) return "";
    return std::format("add_label_with_icon|small|`wCurse`` (`w{}`` left)|left|{}|\n", curse_time_left(p), curse_icon());
}