#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/NameChanged.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "mod.hpp"

void mod(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    // text arrives as "mod {name} {level}" - drop the command word
    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /mod {name} {0=player, 1=mod, 2=dev}``");
        return;
    }

    std::string target_name{}, level_str{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        target_name = arg.substr(0, sp);
        level_str = arg.substr(sp + 1);
    }
    else
    {
        on::ConsoleMessage(event.peer, "`4Usage: /mod {name} {0=player, 1=mod, 2=dev}``");
        return;
    }

    std::erase_if(level_str, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
    if (level_str.empty())
    {
        on::ConsoleMessage(event.peer, "`4Level must be 0, 1 or 2.``");
        return;
    }

    signed level{};
    try { level = std::stoi(level_str); }
    catch (...) { on::ConsoleMessage(event.peer, "`4Level must be 0, 1 or 2.``"); return; }

    if (level < 0 || level > 2)
    {
        on::ConsoleMessage(event.peer, "`4Level must be 0, 1 or 2.``");
        return;
    }

    // lowercase the target since growid is stored lowercase-insensitive
    std::string lower_name = target_name;
    for (char &c : lower_name) c = std::tolower(static_cast<unsigned char>(c));

    // make sure the account exists
    {
        ::hStmt hStmt{ "SELECT 1 FROM peer WHERE growid = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(lower_name);
        hStmt.bind_param(&param);
        hStmt.execute();
        if (mysql_stmt_store_result(hStmt.pStmt) || mysql_stmt_num_rows(hStmt.pStmt) == 0)
        {
            on::ConsoleMessage(event.peer, std::format("`4No account named `w{}`` exists.``", target_name));
            return;
        }
    }

    // write the new role
    {
        ::hStmt hStmt{ "UPDATE peer SET role = ? WHERE growid = ?" };
        MYSQL_BIND params[2] = { make_bind_in(level), make_bind_in(lower_name) };
        hStmt.bind_param(params);
        hStmt.execute();
    }

    static constexpr const char *labels[3] = { "Player", "Moderator", "Developer" };

    // if they're online, update them live
    bool online = false;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget) return;

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name != lower_name) return;

        online = true;
        pTarget->role = static_cast<u_char>(level);
        pTarget->display_growid = (pTarget->role >= DEVELOPER) ? std::format("`6@{}``", pTarget->growid) : (pTarget->role >= MODERATOR) ? std::format("`5@{}``", pTarget->growid) : std::format("`w{}``", pTarget->growid);

        on::ConsoleMessage(&p, std::format("`2You are now a `w{}``.``", labels[level]));
    });

    on::ConsoleMessage(event.peer,
        std::format("`2Set `w{}`` to `w{}``{}.``", target_name, labels[level],
                    online ? "" : " (offline, applies on next login)"));
}
