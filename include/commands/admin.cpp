#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/NameChanged.hpp"
#include "database/database.hpp"

#include "admin.hpp"

/* find online peer by growid (case-insensitive) */
static ::peer* find_peer_by_growid(const std::string_view growid)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state != ENET_PEER_STATE_CONNECTED) continue;
        ::peer *pp = static_cast<::peer*>(p.data);
        if (!pp) continue;

        // case-insensitive compare
        if (pp->growid.size() == growid.size())
        {
            bool match = true;
            for (std::size_t i = 0; i < growid.size(); ++i)
                if (std::tolower(pp->growid[i]) != std::tolower(growid[i]))
                    { match = false; break; }
            if (match) return pp;
        }
    }
    return nullptr;
}

/* extract 2nd word from text after command name */
static std::string get_arg(const std::string_view text, int n = 1)
{
    auto parts = readch(std::string(text), ' ');
    if (parts.size() > (size_t)n) return parts[n];
    return "";
}

void give(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    auto parts = readch(std::string(text), ' ');
    if (parts.size() < 4)
    {
        on::ConsoleMessage(event.peer, "Usage: /give {growid} {itemid} {count}");
        return;
    }

    std::string target_name = parts[1];
    short item_id = (short)std::atoi(parts[2].c_str());
    short count = (short)std::atoi(parts[3].c_str());

    if (item_id <= 0 || count <= 0 || count > 200)
    {
        on::ConsoleMessage(event.peer, "`4Invalid item ID or count (1-200).``");
        return;
    }

    ::peer *target = find_peer_by_growid(target_name);
    if (!target)
    {
        on::ConsoleMessage(event.peer, std::format("`4Player '{}' not found or offline.``", target_name));
        return;
    }

    // create a fake event pointing to target peer
    ENetEvent fake{ .peer = static_cast<ENetPeer*>(nullptr) };
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
            { fake.peer = &p; break; }

    if (!fake.peer)
    {
        on::ConsoleMessage(event.peer, "`4Failed to locate target peer.``");
        return;
    }

    u_short excess = modify_item_inventory(fake, ::slot{item_id, count});
    if (excess > 0)
        on::ConsoleMessage(event.peer, std::format("`5Gave {} {}x{} ({} excess).``", target->growid, item_id, count, excess));
    else
        on::ConsoleMessage(event.peer, std::format("`5Gave {} {}x{}.``", target->growid, item_id, count));
}

void setgems(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    auto parts = readch(std::string(text), ' ');
    if (parts.size() < 3)
    {
        on::ConsoleMessage(event.peer, "Usage: /setgems {growid} {amount}");
        return;
    }

    std::string target_name = parts[1];
    int amount = std::atoi(parts[2].c_str());

    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        target->gems = amount;
        // notify target
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
            {
                send_varlist(&p, { "OnSetBux", amount });
                on::ConsoleMessage(&p, std::format("`5Your gems have been set to {}.``", amount));
                break;
            }
        }
        on::ConsoleMessage(event.peer, std::format("`5Set {}'s gems to {}.``", target->growid, amount));
    }

    // also update DB directly
    ::hStmt hStmt{ "UPDATE peer SET gems = ? WHERE growid = ?" };
    MYSQL_BIND params[2] = { make_bind_in(amount), make_bind_in(target_name) };
    mysql_stmt_bind_param(hStmt.pStmt, params);
    mysql_stmt_execute(hStmt.pStmt);
}

void ban(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    std::string target_name = get_arg(text);
    if (target_name.empty())
    {
        on::ConsoleMessage(event.peer, "Usage: /ban {growid}");
        return;
    }

    // update DB
    {
        ::hStmt hStmt{ "UPDATE peer SET banned = 1 WHERE growid = ?" };
        MYSQL_BIND param = make_bind_in(target_name);
        mysql_stmt_bind_param(hStmt.pStmt, &param);
        mysql_stmt_execute(hStmt.pStmt);
    }

    // kick if online
    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        target->banned = true;
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
            {
                send_action(p, "log", "msg|`4You have been banned.``");
                enet_peer_disconnect_later(&p, 0);
                break;
            }
        }
    }
    on::ConsoleMessage(event.peer, std::format("`5Banned {}.``", target_name));
}

void unban(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    std::string target_name = get_arg(text);
    if (target_name.empty())
    {
        on::ConsoleMessage(event.peer, "Usage: /unban {growid}");
        return;
    }

    ::hStmt hStmt{ "UPDATE peer SET banned = 0 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    on::ConsoleMessage(event.peer, std::format("`5Unbanned {}.``", target_name));
}

void mute(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    std::string target_name = get_arg(text);
    if (target_name.empty())
    {
        on::ConsoleMessage(event.peer, "Usage: /mute {growid}");
        return;
    }

    // update DB
    {
        ::hStmt hStmt{ "UPDATE peer SET muted = 1 WHERE growid = ?" };
        MYSQL_BIND param = make_bind_in(target_name);
        mysql_stmt_bind_param(hStmt.pStmt, &param);
        mysql_stmt_execute(hStmt.pStmt);
    }

    // set in-memory if online
    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        target->muted = true;
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
            {
                on::ConsoleMessage(&p, "`4You have been muted.``");
                break;
            }
        }
    }
    on::ConsoleMessage(event.peer, std::format("`5Muted {}.``", target_name));
}

void unmute(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    std::string target_name = get_arg(text);
    if (target_name.empty())
    {
        on::ConsoleMessage(event.peer, "Usage: /unmute {growid}");
        return;
    }

    ::hStmt hStmt{ "UPDATE peer SET muted = 0 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    ::peer *target = find_peer_by_growid(target_name);
    if (target) target->muted = false;

    on::ConsoleMessage(event.peer, std::format("`5Unmuted {}.``", target_name));
}

void kick(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Permission denied.``");
        return;
    }

    std::string target_name = get_arg(text);
    if (target_name.empty())
    {
        on::ConsoleMessage(event.peer, "Usage: /kick {growid}");
        return;
    }

    ::peer *target = find_peer_by_growid(target_name);
    if (!target)
    {
        on::ConsoleMessage(event.peer, std::format("`4Player '{}' not found or offline.``", target_name));
        return;
    }

    // can't kick higher role unless you're dev
    if (pPeer->role <= target->role && pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Cannot kick a player with equal or higher role.``");
        return;
    }

    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
        {
            on::ConsoleMessage(&p, "`4You have been kicked.``");
            enet_peer_disconnect_later(&p, 0);
            break;
        }
    }
    on::ConsoleMessage(event.peer, std::format("`5Kicked {}.``", target_name));
}

void setrole(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Developer only.``");
        return;
    }

    auto parts = readch(std::string(text), ' ');
    if (parts.size() < 3)
    {
        on::ConsoleMessage(event.peer, "Usage: /setrole {growid} {0|1|2}  (0=Player, 1=Mod, 2=Dev)");
        return;
    }

    std::string target_name = parts[1];
    u_char new_role = (u_char)std::atoi(parts[2].c_str());
    if (new_role > DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Invalid role. 0=Player, 1=Mod, 2=Dev``");
        return;
    }

    // update DB
    {
        ::hStmt hStmt{ "UPDATE peer SET role = ? WHERE growid = ?" };
        int role_int = new_role;
        MYSQL_BIND params[2] = { make_bind_in(role_int), make_bind_in(target_name) };
        mysql_stmt_bind_param(hStmt.pStmt, params);
        mysql_stmt_execute(hStmt.pStmt);
    }

    // update online player
    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        target->role = new_role;
        target->prefix = (new_role == MODERATOR) ? "#@" : (new_role == DEVELOPER) ? "8@" : "w";

        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && static_cast<::peer*>(p.data) == target)
            {
                ENetEvent fake{ .peer = &p };
                on::NameChanged(fake);
                on::ConsoleMessage(&p, std::format("`5Your role has been set to {}.``",
                    new_role == DEVELOPER ? "Developer" : new_role == MODERATOR ? "Moderator" : "Player"));
                break;
            }
        }
    }

    on::ConsoleMessage(event.peer, std::format("`5Set {}'s role to {}.``", target_name, new_role));
}