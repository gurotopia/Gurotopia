#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "action/respawn.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "events.hpp"
#include "useitems.hpp"
#include "buffs.hpp"
#include "godmode.hpp"
#include "dropqueue.hpp"
#include "euphoria.hpp"
#include "shutdown.hpp"
#include "legendary.hpp"
#include "wands.hpp"

static constexpr short FREEZE_WAND = 274;
static constexpr short FIRE_WAND   = 276;
static constexpr short CURSE_WAND  = 278;
static constexpr short BAN_WAND    = 732;

/* the player standing on tile (x, y) in this world (not counting skip), or nullptr */
static ENetPeer *player_at(const std::string &world, int x, int y, const ::peer *skip)
{
    ENetPeer *hit = nullptr;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t == skip || hit) return;
        const int x0 = static_cast<int>(std::floor(t->pos.x / 32.0f));
        const int x1 = static_cast<int>(std::floor((t->pos.x + 19.0f) / 32.0f));
        const int y0 = static_cast<int>(std::floor(t->pos.y / 32.0f));
        const int y1 = static_cast<int>(std::floor((t->pos.y + 29.0f) / 32.0f));
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) hit = &p;
    });
    return hit;
}

static void announce(const std::string &text)
{
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *o = static_cast<::peer*>(p.data);
        if (o && !o->growid.empty()) on::ConsoleMessage(&p, text);
    });
}

bool wand_use(ENetEvent& event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    const short id = static_cast<short>(item.id);
    if (id != FREEZE_WAND && id != FIRE_WAND && id != CURSE_WAND && id != BAN_WAND) return false;

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR)
    {
        tell(event.peer, "`4Only staff can use this.``");
        return true;
    }
    ENetPeer *found = player_at(world.name, static_cast<int>(gamePacket.punch.x), static_cast<int>(gamePacket.punch.y), pPeer);
    if (!found) found = player_at(world.name, static_cast<int>(gamePacket.punch.x), static_cast<int>(gamePacket.punch.y), nullptr); // @note yourself
    if (!found)
    {
        tell(event.peer, "`4Use that on a player.``");
        return true;
    }
    ::peer *pTarget = static_cast<::peer*>(found->data);

    if (pTarget->role > pPeer->role)
    {
        tell(event.peer, "`4That player is immune to your wand.``");
        return true;
    }

    const std::string name = pTarget->growid;
    const std::time_t now = std::time(nullptr);

    switch (id)
    {
        case FIRE_WAND:
        {
            ENetEvent ev{};
            ev.peer = found;
            action::respawn(ev, "");
            on::ConsoleMessage(found, std::format("`w{}`` burned you with a `4Fire Wand``!", pPeer->growid));
            on::ConsoleMessage(event.peer, std::format("`2You burned `w{}``.``", name));
            break;
        }
        case FREEZE_WAND:
        {
            if (pTarget->frozen && pTarget->freeze_until == 0)
            {
                on::ConsoleMessage(event.peer, std::format("`w{}`` is already frozen.", name));
                return true;
            }
            pTarget->frozen = true;
            pTarget->state |= S_FROZEN;
            pTarget->freeze_until = now + 10;
            on::SetClothing(*found);
            send_varlist(found, { "OnSetFreezeState", 1u }, pTarget->netid);
            on::ConsoleMessage(found, std::format("`w{}`` froze you with a `1Freeze Wand``!", pPeer->growid));
            on::ConsoleMessage(event.peer, std::format("`2You froze `w{}`` for 10 seconds.``", name));
            break;
        }
        case CURSE_WAND:
        {
            pTarget->curse_until = now + 600;
            pTarget->mysql_update("curse_until", static_cast<signed>(pTarget->curse_until));
            announce(std::format("`#** `$The Ancient Ones`` have used `#Curse`` on `w{}``! `#**``", name));
            tell(found, "`4You have been cursed`` for `w10 mins``. Reason: `wCurse Wand``");
            on::ConsoleMessage(event.peer, std::format("`2Cursed `w{}`` for 10 minutes.``", name));
            if (pTarget->recent_worlds.back() != "HELL")
            {
                ENetEvent ev{};
                ev.peer = found;
                action::quit_to_exit(ev, "", true);
                action::join_request(ev, "", "HELL");
            }
            else on::SetClothing(*found);
            break;
        }
        case BAN_WAND:
        {
            pTarget->ban_until = now + 600;
            pTarget->mysql_update("ban_until", static_cast<signed>(pTarget->ban_until));
            announce(std::format("`#** `$The Ancient Ones`` have `4banned`` `w{}``! `#**``", name));
            tell(found, "`4You have been banned`` for `w10 minutes``.");
            on::ConsoleMessage(event.peer, std::format("`2Banned `w{}`` for 10 minutes.``", name));
            enet_peer_disconnect_later(found, 0);
            break;
        }
    }
    modify_item_inventory(event, ::slot(id, -1)); // @note wands are used up
    return true;
}

/* runs about once a second: ends wand freezes and expired curses */
void tick_timers()
{
    static std::time_t last = 0;
    const std::time_t now = std::time(nullptr);
    dropqueue_pump();
    leave_pump(); // @note every loop, not just once a second
    if (now == last) return;
    last = now;
    events_tick(now);
    euphoria_tick(now);
    shutdown_tick(now);
    legend_tick(now);
    useitems_tick(now);
    buffs_tick(now);
    god_rejoin_tick(now);

    peers("", peer_condition::PEER_ALL, [now](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;

        if (t->freeze_until != 0 && now >= t->freeze_until)
        {
            t->freeze_until = 0;
            t->frozen = false;
            t->state &= ~S_FROZEN;
            if (t->netid != 0)
            {
                on::SetClothing(p);
                send_varlist(&p, { "OnSetFreezeState", 0u }, t->netid);
            }
            on::ConsoleMessage(&p, "`2You can move again.``");
        }

        if (t->curse_until != 0 && now >= t->curse_until)
        {
            t->curse_until = 0;
            t->mysql_update("curse_until", static_cast<signed>(0));
            if (t->netid != 0) on::SetClothing(p);
            on::ConsoleMessage(&p, "`2Your curse has worn off!`` Sending you back...");
            if (t->netid != 0 && t->recent_worlds.back() == "HELL")
            {
                ENetEvent ev{};
                ev.peer = &p;
                action::quit_to_exit(ev, "", false); // @note back to the world menu
            }
        }
    });
}
