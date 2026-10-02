#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "action/respawn.hpp"
#include "tools/bubble.hpp"
#include "tools/random.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "buffs.hpp"
#include "events.hpp"
#include "extra_items.hpp"

static int count_of(const ::peer &p, short id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}

static void bubble(const ::peer &who, const std::string &text)
{
    const ::peer &s = speaker(who); // @note used on another player: they say it
    peers(s.recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { send_varlist(&p, { "OnTalkBubble", s.netid, text, 0u }); });
}

static ENetPeer *player_at(const std::string &world, int x, int y, const ::peer *skip)
{
    ENetPeer *hit = nullptr;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t == skip || hit) return;
        const int x0 = static_cast<int>(std::floor(t->pos.x / 32.0f)), x1 = static_cast<int>(std::floor((t->pos.x + 19.0f) / 32.0f));
        const int y0 = static_cast<int>(std::floor(t->pos.y / 32.0f)), y1 = static_cast<int>(std::floor((t->pos.y + 29.0f) / 32.0f));
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) hit = &p;
    });
    return hit;
}

static void add_state(::peer &p, int flag, int seconds)
{
    const std::time_t ends = std::time(nullptr) + seconds;
    for (auto &e : p.item_states) if (e.first == flag) { e.second = ends; return; }
    p.item_states.emplace_back(flag, ends);
}

struct combine { short from; int need; short into; };
static constexpr combine combines[]{
    { 1234, 4,  1206 }, // @note Devil Wing Fragment x4 -> Devil Wings
    { 3122, 16, 3120 }, // @note Teeny Wing Fragment x16 -> Teeny Devil Wings
    { 3318, 100, 3336 }, { 3320, 100, 3338 }, { 3322, 100, 3340 }, // @note Silk Thread x100 -> Silk Bolt (red, green, blue)
    { 3324, 100, 3342 }, { 3326, 100, 3344 }, { 3328, 100, 3346 }, // @note black, white, grey
    { 3330, 100, 3348 }, { 3332, 100, 3350 }, { 3334, 100, 3352 }, // @note yellow, purple, aqua
    { 3890, 100, 3892 }, // @note Wool x100 -> Wool Bolt
};

bool extra_item_use(ENetEvent &event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const short id = static_cast<short>(item.id);
    const int x = static_cast<int>(gamePacket.punch.x), y = static_cast<int>(gamePacket.punch.y);
    const int width = static_cast<int>(world.blocks.size() / 60);
    auto use_one = [&] { modify_item_inventory(event, ::slot(id, -1)); pPeer->add_xp(event, consumable_xp(*pPeer)); };

    for (const combine &c : combines)
        if (id == c.from)
        {
            const int have = count_of(*pPeer, c.from);
            const std::string into = id_to_item(static_cast<u_short>(c.into)).raw_name;
            if (have < c.need)
            {
                tell(event.peer, std::format("`4You need `w{}`` of these to make `w{}`` (you have {}).``", c.need, into, have));
                return true;
            }
            modify_item_inventory(event, ::slot(c.from, static_cast<short>(-c.need)));
            modify_item_inventory(event, ::slot(c.into, 1));
            tell(event.peer, std::format("`2You made `w{}``!``", into));
            return true;
        }

    switch (id)
    {
        case 2966: // @note Enchanted Spatula: flip which way a block faces
        {
            if (x < 0 || y < 0 || x >= width || y >= 60) return true;
            ::block &b = world.blocks[cord(x, y)];
            if (b.fg == 0) { tell(event.peer, "`4Slap a block with it.``"); return true; }
            b.state[2] ^= S_LEFT;
            send_tile_update(event, { .id = b.fg, .punch = gamePacket.punch }, b, world);
            modify_item_inventory(event, ::slot(id, -1));
            bubble(*pPeer, "*SLAP!*");
            tell(event.peer, "`oThe spatula shatters.``");
            return true;
        }
        case 3102: // @note Eldritch Flame: fires all over the world
        {
            use_one();
            int lit = 0;
            for (int tries = 0; tries < 600 && lit < 15; ++tries)
            {
                const int fx = RandomRange(0, width), fy = RandomRange(0, 54);
                ::block &b = world.blocks[cord(fx, fy)];
                if ((b.fg == 0 && b.bg == 0) || (b.state[3] & (S_FIRE | S_WATER)) || b.fg == 6 || b.fg == 8) continue;
                b.state[3] |= S_FIRE;
                send_tile_update(event, { .id = b.fg, .punch = ::pos{ fx, fy } }, b, world);
                ++lit;
            }
            peers(world.name, PEER_SAME_WORLD, [&](ENetPeer &p)
            {
                on::ConsoleMessage(&p, std::format("{} `4unleashed the Eldritch Flame!`` `w{}`` fires broke out!", pPeer->display_growid, lit));
            });
            return true;
        }
        case 8520: // @note Dangerous Pineapple: most don't survive it
        {
            use_one();
            if (RandomRange(0, 100) < 75)
            {
                bubble(*pPeer, "`4*chokes*``");
                action::respawn(event, "");
            }
            else bubble(*pPeer, "YUM!");
            return true;
        }
        case 1964: // @note Devilfruit: devil horns for an hour
        {
            use_one();
            add_state(*pPeer, 0x40, 3600);
            on::SetClothing(*event.peer);
            bubble(*pPeer, "`4*feels evil*``");
            return true;
        }
        case 2306: // @note Party-In-A-Box
        {
            if (!event_party(world))
            {
                tell(event.peer, "`4The party couldn't start here right now.``");
                return true;
            }
            modify_item_inventory(event, ::slot(id, -1));
            return true;
        }
        case 7672: // @note Golden Party-In-A-Box: one random prize
        {
            static constexpr std::pair<short, short> prizes[]{
                { 7668, 2 }, { 7662, 3 }, { 7666, 5 }, { 11574, 1 }, { 10568, 1 }, { 11572, 3 }, { 7664, 3 }, { 13000, 1 },
                { 12996, 1 }, { 14166, 1 }, { 10534, 1 }, { 14172, 1 }, { 14168, 1 }, { 14170, 1 }, { 13030, 1 }, { 15132, 1 },
                { 15116, 1 }, { 11534, 1 }, { 9256, 1 }, { 15118, 1 }, { 14152, 1 }, { 11544, 1 }, { 10492, 1 }, { 9262, 1 },
                { 13002, 1 }, { 7660, 1 }, { 10494, 1 }, { 4366, 1 }, { 1406, 1 },
            };
            const auto &[prize, qty] = prizes[RandomRange(0, static_cast<int>(std::size(prizes)))];
            modify_item_inventory(event, ::slot(id, -1));
            modify_item_inventory(event, ::slot(prize, qty));
            tell(event.peer, std::format("`2The Golden Party-In-A-Box contained `w{}x {}``!``", qty, id_to_item(static_cast<u_short>(prize)).raw_name));
            return true;
        }
        case 2288: // @note Party Confetti: 100 at once gets the party going
        {
            const int have = count_of(*pPeer, 2288);
            if (have < 100)
            {
                tell(event.peer, std::format("`4It takes `w100`` Party Confetti to get the party started (you have {}).``", have));
                return true;
            }
            modify_item_inventory(event, ::slot(2288, -100));
            bubble(*pPeer, "`2*CONFETTI EVERYWHERE*``");
            return true;
        }
        case 9264: // @note Anniversary Skyrocket
        {
            use_one();
            fireworks(event, ::pos{ x * 32.0f + 16.0f, y * 32.0f + 16.0f });
            return true;
        }
        case 15650: // @note Mystery Lens: Hidden in Plain Sight
        {
            if (!event_hidden_object(world))
            {
                tell(event.peer, "`4Nothing could be hidden here right now.``");
                return true;
            }
            modify_item_inventory(event, ::slot(id, -1));
            return true;
        }
        case 5706: // @note Small Seed Pack: 4 common seeds (rarity 2-12) + 1 rare (13-21)
        {
            static std::vector<short> common{}, rare{};
            if (common.empty())
                for (const ::item &it : items)
                {
                    if (it.type != type::SEED || it.raw_name.empty()) continue;
                    if (it.rarity >= 2 && it.rarity <= 12) common.push_back(static_cast<short>(it.id));
                    else if (it.rarity >= 13 && it.rarity <= 21) rare.push_back(static_cast<short>(it.id));
                }
            if (common.empty() || rare.empty()) return true;
            modify_item_inventory(event, ::slot(id, -1));
            std::string got{};
            for (int i = 0; i < 5; ++i)
            {
                const short seed = (i < 4) ? common[RandomRange(0, static_cast<int>(common.size()))] : rare[RandomRange(0, static_cast<int>(rare.size()))];
                modify_item_inventory(event, ::slot(seed, 1));
                got += std::format("{}`w{}``", got.empty() ? "" : ", ", id_to_item(static_cast<u_short>(seed)).raw_name);
            }
            on::ConsoleMessage(event.peer, std::format("`2The Small Seed Pack contained:`` {}", got));
            return true;
        }
        case 4754: // @note Super Exploding Pineapple: kills everyone within 2 tiles
        {
            use_one();
            const ::pos at{ x * 32.0f + 16.0f, y * 32.0f + 16.0f };
            for (int i = 0; i < 5; ++i) fireworks(event, at);
            std::vector<ENetPeer*> hit{};
            peers(world.name, PEER_SAME_WORLD, [&](ENetPeer &p)
            {
                ::peer *t = static_cast<::peer*>(p.data);
                if (!t) return;
                const int tx = static_cast<int>((t->pos.x + 10.0f) / 32.0f), ty = static_cast<int>((t->pos.y + 15.0f) / 32.0f);
                if (std::abs(tx - x) <= 2 && std::abs(ty - y) <= 2) hit.push_back(&p);
            });
            for (ENetPeer *p : hit)
            {
                ENetEvent ev{};
                ev.peer = p;
                action::respawn(ev, "");
            }
            peers(world.name, PEER_SAME_WORLD, [&](ENetPeer &p)
            {
                on::ConsoleMessage(&p, std::format("`4KABOOM!`` {}`4's Super Exploding Pineapple went off!``", pPeer->display_growid));
            });
            return true;
        }
        case 540: // @note Green Beer: turns you green for a while
        {
            use_one();
            pPeer->tint = 0x40D040FFu;
            pPeer->tint_item = 540;
            pPeer->tint_until = std::time(nullptr) + 600;
            on::SetClothing(*event.peer);
            bubble(*pPeer, "YUM!");
            return true;
        }
        case 1988: // @note Doppelganger Potion: haunted shadows for 10 minutes
        {
            ENetPeer *found = player_at(world.name, x, y, pPeer);
            if (!found) found = player_at(world.name, x, y, nullptr);
            if (!found) { tell(event.peer, "`4Throw it on a player.``"); return true; }
            ::peer *t = static_cast<::peer*>(found->data);
            if (t->role > pPeer->role) { tell(event.peer, "`4That player is immune.``"); return true; }
            use_one();
            add_state(*t, 0x40000, 600);
            on::SetClothing(*found);
            bubble(*t, "`8*the past comes back to haunt them*``");
            return true;
        }
    }
    return false;
}
