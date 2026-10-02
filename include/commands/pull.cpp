#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "database/world.hpp"
#include "pull.hpp"

static std::string lower_of(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

ENetPeer *find_in_world(const std::string &world, const std::string &name, bool &ambiguous)
{
    ambiguous = false;
    const std::string want = lower_of(name);
    ENetPeer *exact = nullptr, *prefix = nullptr;
    int prefixes = 0;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;
        const std::string have = lower_of(t->growid);
        if (have == want) exact = &p;
        else if (want.size() >= 3 && have.starts_with(want)) { prefix = &p; ++prefixes; }
    });
    if (exact) return exact;
    if (prefixes > 1) ambiguous = true;
    return prefixes == 1 ? prefix : nullptr;
}

bool pull_player(ENetEvent &event, ENetPeer &target)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);
    ::peer *t = static_cast<::peer*>(target.data);
    if (me->netid == 0) return false;

    auto world = std::ranges::find(worlds, me->recent_worlds.back(), &::world::name);
    const bool staff = me->role >= MODERATOR;
    const bool owner = world != worlds.end() && world->owner != 0 && world->owner == me->user_id;
    if (!staff && !owner)
    {
        tell(event.peer, "`4Only the world owner can pull players.``");
        return false;
    }
    if (t == me) { tell(event.peer, "`4You can't pull yourself.``"); return false; }
    if (t->netid == 0 || t->recent_worlds.back() != me->recent_worlds.back())
    {
        tell(event.peer, "`4They have to be in this world.``");
        return false;
    }
    if (t->role >= MODERATOR && t->role >= me->role)
    {
        tell(event.peer, "`4You can't pull staff.``");
        return false;
    }

    t->pos = me->pos;
    send_varlist(&target, { "OnSetPos", CL_Vec2f{ me->pos.x, me->pos.y } }, t->netid);
    const std::string who = (me->role >= DEVELOPER) ? "`6Dev``" : (me->role >= MODERATOR) ? "`5Mod``" : "`2world owner``";
    send_varlist(&target, { "OnAddNotification", "interface/atomic_button.rttex", std::format("`wYou were pulled by a {}!``", who), "audio/hub_open.wav", 0u }); // @note popup like /summon
    on::ConsoleMessage(&target, std::format("`5You were pulled by a {}.``", who));
    on::ConsoleMessage(event.peer, std::format("`2Pulled `w{}``.``", t->growid));
    return true;
}

/* /pull {player} - the first letters are enough */
void pull_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);
    if (me->netid == 0) { tell(event.peer, "`4You must be in a world.``"); return; }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();
    if (arg.empty()) { tell(event.peer, "`4Usage: /pull {player}`` - the first 3 letters are enough."); return; }

    bool ambiguous = false;
    ENetPeer *found = find_in_world(me->recent_worlds.back(), arg, ambiguous);
    if (!found)
    {
        tell(event.peer, ambiguous ? "`4More than one player starts with that - type more letters.``"
                                   : std::format("`4Nobody called `w{}`` is in this world.``", arg));
        return;
    }
    pull_player(event, *found);
}
