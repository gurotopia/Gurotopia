#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "whois.hpp"

/* /whois {player} -> account details for anyone, online or not */
void whois(ENetEvent& event, const std::string_view text)
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

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /whois {player}``");
        return;
    }

    std::string lower_name = arg;
    for (char &c : lower_name) c = std::tolower(static_cast<unsigned char>(c));

    signed uid = 0, role = 0, gems = 0;
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

    {
        ::hStmt hStmt{ "SELECT role FROM peer WHERE uid = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(uid);
        hStmt.bind_param(&param);
        MYSQL_BIND result = make_bind_out(role);
        mysql_stmt_bind_result(hStmt.pStmt, &result);
        hStmt.execute();
        hStmt.fetch();
    }

    {
        ::hStmt hStmt{ "SELECT gems FROM peer WHERE uid = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(uid);
        hStmt.bind_param(&param);
        MYSQL_BIND result = make_bind_out(gems);
        mysql_stmt_bind_result(hStmt.pStmt, &result);
        hStmt.execute();
        hStmt.fetch();
    }

    static constexpr const char *labels[3] = { "Player", "Moderator", "Developer" };
    const char *label = (role >= 0 && role <= 2) ? labels[role] : "Unknown";

    // are they online?
    std::string where = "`4offline``";
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget || pTarget->user_id != uid) return;
        where = (pTarget->netid == 0)
            ? "`2online`` `o(world menu)"
            : std::format("`2online`` `oin `2{}``", pTarget->recent_worlds.back());
    });

    on::ConsoleMessage(event.peer, std::format("`o--- `2{}`o ---", arg));
    on::ConsoleMessage(event.peer, std::format("`oUser id: `w{}``", uid));
    on::ConsoleMessage(event.peer, std::format("`oRole: `w{}``", label));
    on::ConsoleMessage(event.peer, std::format("`oGems: `w{}``", gems));
    on::ConsoleMessage(event.peer, std::format("`oStatus: {}", where));
}
