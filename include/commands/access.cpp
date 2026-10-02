#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "lockaccess.hpp"
#include "access.hpp"

static std::string lower_of(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/* an account number from a name: players in this world first (exact, or the first letters), then any account */
static int resolve_uid(const std::string &arg, const std::string &world)
{
    const std::string want = lower_of(arg);
    int exact = 0, prefix = 0, prefixes = 0;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;
        const std::string name = lower_of(t->growid);
        if (name == want) exact = t->user_id;
        else if (want.size() >= 3 && name.starts_with(want)) { prefix = t->user_id; ++prefixes; }
    });
    if (exact) return exact;
    if (prefixes == 1) return prefix;

    signed uid = 0;
    ::hStmt hStmt{ "SELECT uid FROM peer WHERE growid = ? LIMIT 1" };
    MYSQL_BIND param = make_bind_in(want);
    hStmt.bind_param(&param);
    MYSQL_BIND result = make_bind_out(uid);
    mysql_stmt_bind_result(hStmt.pStmt, &result);
    hStmt.execute();
    hStmt.fetch();
    return uid;
}

/* /access            -> lists who has access to this world
 * /access {player}   -> gives that player access, or takes it away if they already have it
 */
void access_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    if (world->owner != pPeer->user_id && pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only the world owner can change access.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg.empty())
    {
        std::string names{};
        for (int uid : world->access)
        {
            if (uid == 0) continue;
            if (!names.empty()) names += "`o, ";
            names += std::format("`w{}``", access_name_of(uid));
        }
        if (names.empty()) on::ConsoleMessage(event.peer, "`oNobody else has access to this world.");
        else on::ConsoleMessage(event.peer, std::format("`oAccess: {}", names));
        return;
    }

    const int uid = resolve_uid(arg, world->name);
    if (uid == 0)
    {
        on::ConsoleMessage(event.peer, std::format("`4No account named `w{}`` exists.``", arg));
        return;
    }

    // same as the World Lock menu: already listed -> remove, otherwise add
    if (!lock_access_remove(event, *world, uid)) lock_access_add(event, *world, uid);
}
