#include "pch.hpp"
#include "tools/bubble.hpp"
#include <fstream>
#include "onVariant/ConsoleMessage.hpp"
#include "tools/time.hpp"
#include "tools/random.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "wands.hpp"
#include "food.hpp"
#include "useitems.hpp"
#include "buffs_items.hpp"
#include "worldtools_use.hpp"
#include "scrolls.hpp"
#include "extra_items.hpp"
#include "euphoria.hpp"
#include "jammers.hpp"
#include "consumables.hpp"

static constexpr short FIREWORKS        = 834;
static constexpr short SUPER_FIREWORKS  = 1680;
static constexpr short FIREWORKS_NEEDED = 200; // @note Fireworks used per Super Fireworks. 0 = no requirement

/* ---------------------------------------------------------- name lookup */

static std::string lower(std::string s)
{
    for (char &c : s) c = std::tolower(static_cast<unsigned char>(c));
    return s;
}

static short id_from_name(const char *name)
{
    const std::string want = lower(name);
    for (const ::item &it : items)
        if (it.raw_name.size() == want.size() && lower(it.raw_name) == want)
            return static_cast<short>(it.id);
    return 0;
}

/* resolved once on first use */
static std::vector<short> grow_items{};
static std::vector<short> firework_items{};
static std::vector<short> super_drops{};
static bool resolved = false;

static void resolve()
{
    if (resolved) return;
    resolved = true;

    for (const char *n : { "Grow Spray Fertilizer", "My First Grow Spray Fertilizer" })
    {
        const short id = id_from_name(n);
        if (id) grow_items.emplace_back(id);
        else (void)0; // @note debug line removed
    }

    /* every consumable with "firework" in the name, except Super Fireworks */
    for (const ::item &it : items)
    {
        if (it.id == SUPER_FIREWORKS || it.type != type::CONSUMEABLE) continue;
        if (lower(it.raw_name).find("firework") != std::string::npos)
            firework_items.emplace_back(static_cast<short>(it.id));
    }

    for (const char *n : {
        "Atomic Fireball", "Barbecue Grill", "Beach Ball", "Beach Blast", "Body Tattoos",
        "Bubble Machine", "Frangipani", "Great Ball of Fire", "Greg, The Octopus", "Growmoji Fireworks",
        "Hydro Cannon", "Long Surfer Hair", "Oceanic Crown", "Pet Toucan", "Pet Turtle",
        "Poseidon's Trident", "Riding Flamingo", "Sandcastle", "Sandtopian", "Sea Monster Floatie",
        "Seafoam Beard", "Seafoam Hair", "Shark Head", "Shark Suit", "Sharkzooka",
        "Short Surfer Hair", "Squirt Gun", "Summer Breeze", "Summer Kite", "Surfboard",
        "Swim Fins", "Water Wings", "Watermelon Slice", "White Fury" })
    {
        const short id = id_from_name(n);
        if (id) super_drops.emplace_back(id);
        else (void)0; // @note debug line removed
    }

    (void)0; // @note debug line removed
}

static bool is_in(const std::vector<short> &list, short id)
{
    return std::ranges::find(list, id) != list.end();
}

/* ---------------------------------------------------------------- usage */

static bool consumable_use_inner(ENetEvent& event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    resolve();
    if (wand_use(event, world, item, gamePacket)) return true;
    if (food_use(event, item)) return true;
    if (item_use(event, world, item, gamePacket)) return true;
    if (buff_use(event, item)) return true;
    if (tool_use(event, world, item, gamePacket)) return true;
    if (scroll_use(event, item)) return true;
    if (extra_item_use(event, world, item, gamePacket)) return true;

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const short id = static_cast<short>(item.id);

    const int x = static_cast<int>(gamePacket.punch.x);
    const int y = static_cast<int>(gamePacket.punch.y);
    if (x < 0 || x >= 100 || y < 0 || y >= 60) return false;

    const ::pos at{ x * 32.0f + 16.0f, y * 32.0f + 16.0f };

    /* ---- fertilizer / grow spray: finish a tree instantly ---- */
    if (is_in(grow_items, id))
    {
        ::block &block = world.blocks[cord(x, y)];
        const ::item &target = id_to_item(block.fg);

        if (target.type != type::SEED)
        {
            tell(event.peer, "`4Use that on a tree.``");
            return true;
        }

        auto tree = std::ranges::find(world.trees, gamePacket.punch, &::tree::pos);
        if (tree == world.trees.end()) return true;

        if (ticks() - tree->tick >= target.tick)
        {
            on::ConsoleMessage(event.peer, "`oThat tree is already fully grown.``");
            return true;
        }

        tree->tick = ticks() - target.tick;
        modify_item_inventory(event, ::slot(id, -1));

        send_tile_update(event, { .id = block.fg, .punch = gamePacket.punch }, block, world);
        send_particle_effect(event, at, { 0x02, 0x61 });

        on::ConsoleMessage(event.peer, std::format("`2The `w{}`` is ready to harvest!``", target.raw_name));
        return true;
    }

    /* ---- Super Fireworks: uses 200 Fireworks, drops a random SummerFest item ---- */
    if (id == SUPER_FIREWORKS)
    {
        if (super_drops.empty())
        {
            tell(event.peer, "`4No Super Fireworks prizes found in items.dat.``");
            return true;
        }

        int have = 0;
        for (const ::slot &s : pPeer->slots)
            if (s.id == FIREWORKS) have += s.count;

        if (have < FIREWORKS_NEEDED)
        {
            on::ConsoleMessage(event.peer, std::format(
                "`4You need {} Fireworks to use Super Fireworks (you have {}).``", FIREWORKS_NEEDED, have));
            return true;
        }

        modify_item_inventory(event, ::slot(SUPER_FIREWORKS, -1));
        if (FIREWORKS_NEEDED > 0) modify_item_inventory(event, ::slot(FIREWORKS, -FIREWORKS_NEEDED));

        for (int i = 0; i < 6; ++i) fireworks(event, at);

        const short prize = super_drops[RandomRange(0, static_cast<int>(super_drops.size()))];
        add_drop(event, ::slot(prize, 1), gamePacket.punch.by_32(), world);

        on::ConsoleMessage(event.peer, std::format("`2The Super Fireworks dropped a `w{}``!``", id_to_item(prize).raw_name));
        return true;
    }

    /* ---- regular fireworks: just the show ---- */
    if (is_in(firework_items, id))
    {
        modify_item_inventory(event, ::slot(id, -1));
        for (int i = 0; i < 3; ++i) fireworks(event, at);
        return true;
    }

    return false; // @note not a consumable we handle - carry on as normal
}


/* wraps the item chain: party items that were really used add Euphoria points */
bool consumable_use(ENetEvent& event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const short id = static_cast<short>(item.id);
    if (pPeer->role < DEVELOPER && jammer_blocks(event, world, id)) return true; // @note Punch / Zombie Jammer
    auto have = [&] { for (const ::slot &s : pPeer->slots) if (s.id == id) return static_cast<int>(s.count); return 0; };
    const int before = have();
    { // @note used on another player? then their head says the item's text
        const int tx = static_cast<int>(gamePacket.punch.x), ty = static_cast<int>(gamePacket.punch.y);
        g_consume_target = nullptr;
        peers(world.name, PEER_SAME_WORLD, [&](ENetPeer &p)
        {
            ::peer *t = static_cast<::peer*>(p.data);
            if (!t || t == pPeer || g_consume_target) return;
            const int x0 = static_cast<int>(std::floor(t->pos.x / 32.0f)), x1 = static_cast<int>(std::floor((t->pos.x + 19.0f) / 32.0f));
            const int y0 = static_cast<int>(std::floor(t->pos.y / 32.0f)), y1 = static_cast<int>(std::floor((t->pos.y + 29.0f) / 32.0f));
            if (tx >= x0 && tx <= x1 && ty >= y0 && ty <= y1) g_consume_target = t;
        });
    }
    struct clear_target { ~clear_target() { g_consume_target = nullptr; } } clear{};
    const bool handled = consumable_use_inner(event, world, item, gamePacket);
    const int used = before - have();
    if (handled && used > 0) euphoria_used(event, world, id, used);
    return handled;
}