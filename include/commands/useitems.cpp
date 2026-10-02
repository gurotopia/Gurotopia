#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/Action.hpp"
#include "action/respawn.hpp"
#include "commands/sb.hpp"
#include "tools/time.hpp"
#include "tools/random.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "punch.hpp"
#include "jammers.hpp"
#include "useitems.hpp"
#include "buffs.hpp"

namespace fx
{
    constexpr int HORNS   = 0x40;
    constexpr int HALO    = 0x80;
    constexpr int STINK   = 0x4000;
    constexpr int SPARKLE = 0x8000;
    constexpr int ZOMBIE  = 0x10000;
}

/* ---------------------------------------------------------- read by SetClothing */

int item_effect_state(const ::peer &p)
{
    const std::time_t now = std::time(nullptr);
    int s = 0;
    for (const auto &[flag, ends] : p.item_states) if (ends > now) s |= flag;
    return s;
}

u_int item_skin(const ::peer &p)
{
    return (p.tint_until > std::time(nullptr)) ? p.tint : p.skin_color;
}

::pos item_speed(const ::peer &p)
{
    if (p.move_until > std::time(nullptr)) return p.move;
    ::pos s{ 250.0f, 1000.0f };
    if (buff_active(p, buff::SPEEDY))    s.x = 350.0f;
    if (buff_active(p, buff::HIGH_JUMP)) s.y = 750.0f;
    if (auto w = std::ranges::find(worlds, p.recent_worlds.back(), &::world::name); w != worlds.end() && w->base_weather == 7) s.y = std::min(s.y, 600.0f); // @note Mars
    if (world_jammer(p.recent_worlds.back(), 4992)) s.y = std::min(s.y, 250.0f); // @note Antigravity Generator
    return s;
}

/* ---------------------------------------------------------- helpers */

static void add_state(::peer &p, int flag, int seconds)
{
    const std::time_t ends = std::time(nullptr) + seconds;
    for (auto &e : p.item_states) if (e.first == flag) { e.second = ends; return; }
    p.item_states.emplace_back(flag, ends);
}

static void refresh(ENetPeer &p)
{
    if (static_cast<::peer*>(p.data)->netid != 0) on::SetClothing(p);
}

static void bubble(const ::peer &who, const std::string &text)
{
    const ::peer &s = speaker(who); // @note used on another player: they say it
    peers(s.recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { send_varlist(&p, { "OnTalkBubble", s.netid, text, 0u }); });
}

static void world_msg(const ::peer &who, const std::string &text)
{
    peers(who.recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { on::ConsoleMessage(&p, text); });
}

static const char *pick(std::initializer_list<const char*> lines)
{
    return *(lines.begin() + RandomRange(0, static_cast<int>(lines.size())));
}

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

/* grow the tree at (x, y) by `add` seconds. @return true if it grew */
static bool grow_tree(ENetEvent &event, ::world &world, int x, int y, u_int add, bool announce)
{
    ::block &block = world.blocks[cord(x, y)];
    const ::item &seed = id_to_item(block.fg);
    if (seed.type != type::SEED)
    {
        if (announce) tell(event.peer, "`4Use that on a tree.``");
        return false;
    }
    auto tree = std::ranges::find(world.trees, ::pos{ x, y }, &::tree::pos);
    if (tree == world.trees.end()) return false;

    const u_int now = static_cast<u_int>(ticks());
    const u_int full = static_cast<u_int>(seed.tick);
    const u_int elapsed = now - tree->tick;
    if (elapsed >= full)
    {
        if (announce) on::ConsoleMessage(event.peer, "`oThat tree is already fully grown.``");
        return false;
    }
    tree->tick = now - std::min<u_int>(elapsed + add, full);
    send_tile_update(event, { .id = block.fg, .punch = ::pos{ x, y } }, block, world);
    return true;
}

/* ---------------------------------------------------------- usage */

bool item_use(ENetEvent &event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const short id = static_cast<short>(item.id);
    const int x = static_cast<int>(gamePacket.punch.x);
    const int y = static_cast<int>(gamePacket.punch.y);
    const std::time_t now = std::time(nullptr);
    auto use_one = [&] { modify_item_inventory(event, ::slot(id, -1)); pPeer->add_xp(event, consumable_xp(*pPeer)); };

    /* ---- tree sprays ---- */
    static const std::unordered_map<short, u_int> tree_sprays{ {228, 3600u}, {10672, 3600u}, {1778, 86400u}, {5764, 86400u}, {1062, 2592000u} };
    if (auto s = tree_sprays.find(id); s != tree_sprays.end())
    {
        if (!grow_tree(event, world, x, y, s->second, true)) return true;
        use_one();
        on::ConsoleMessage(event.peer, std::format("`2You sprayed the `w{}``!``", id_to_item(world.blocks[cord(x, y)].fg).raw_name));
        return true;
    }
    if (id == 13574 || id == 12600) // @note Deluxe / Ultra World Spray
    {
        const u_int add = (id == 12600) ? 2592000u : 86400u;
        int grown = 0;
        for (const ::tree &t : world.trees)
            if (grow_tree(event, world, static_cast<int>(t.pos.x), static_cast<int>(t.pos.y), add, false)) ++grown;
        use_one();
        world_msg(*pPeer, std::format("{} `2sprayed the world - `w{}`` trees grew!``", pPeer->display_growid, grown));
        return true;
    }

    /* ---- celebrations: a fireworks show on the tile ---- */
    if (id == 1066 || id == 1406 || id == 1826 || id == 2236 || id == 4370)
    {
        use_one();
        const ::pos at{ x * 32.0f + 16.0f, y * 32.0f + 16.0f };
        for (int i = 0; i < 3; ++i) fireworks(event, at);
        return true;
    }

    /* ---- used on yourself / a tile ---- */
    switch (id)
    {
        case 752: use_one(); bubble(*pPeer, pick({ "`wHeads!``", "`wTails!``" })); return true;                         // Flipping Coin
        case 3536: use_one(); on::ConsoleMessage(event.peer, "`oYou feel exactly the same.``"); return true;             // Homeopathic Medicine
        case 4574: use_one(); bubble(*pPeer, pick({ "*gags*", "Why would anyone cook this...", "`4BLEGH!``" })); return true; // Disgusting Mess
        case 4752: use_one(); bubble(*pPeer, "YUM!"); return true;                 // Super Pineapple
        case 4378: use_one(); bubble(*pPeer, "YUM!"); on::Action(event, "dance"); return true;                     // Party Cake
        case 4366:                                                                                                         // Party Screamer
        {
            if (pPeer->curse_until > now) { tell(event.peer, "`4You can't do that while cursed.``"); return true; }
            use_one();
            sb(event, std::string{ "sb " } + pick({ "PARTY TIME!!!", "WOOOOO! LET'S GET THIS PARTY STARTED!", "EVERYBODY DANCE NOW!", "THE PARTY IS OFF THE CHIZZY!!" }));
            return true;
        }
        case 4766:                                                                                                         // Cherry
        {
            use_one();
            pPeer->tint = 0x3F3FFFFFu; pPeer->tint_item = 4766;
            pPeer->tint_until = now + 600;
            refresh(*event.peer);
            bubble(*pPeer, "YUM!");
            return true;
        }
        case 782:                                                                                                          // Antidote
        {
            use_one();
            std::erase_if(pPeer->item_states, [](const auto &e) { return e.first == fx::ZOMBIE || e.first == fx::STINK; });
            pPeer->tint_until = 0;
            pPeer->move_until = 0;
            refresh(*event.peer);
            on::ConsoleMessage(event.peer, "`2You feel cured!``");
            return true;
        }
        case 3064:                                                                                                         // Water Balloon
        {
            use_one();
            const int width = static_cast<int>(world.blocks.size() / 60);
            int out = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    const int bx = x + dx, by = y + dy;
                    if (bx < 0 || by < 0 || bx >= width || by >= 60) continue;
                    ::block &b = world.blocks[cord(bx, by)];
                    if (!(b.state[3] & S_FIRE)) continue;
                    ::gamePacket gp{ .punch = ::pos{ bx, by } };
                    remove_fire(event, gp, b, world);
                    ++out;
                }
            bubble(*pPeer, out ? "*SPLASH!*" : "*splash*");
            return true;
        }
    }

    /* ---- used on a player ---- */
    static constexpr std::array<short, 14> targeted{ 126, 128, 196, 338, 368, 384, 388, 614, 616, 618, 764, 874, 962, 1368 };
    if (std::ranges::find(targeted, id) == targeted.end()) return false;

    ENetPeer *found = player_at(world.name, x, y, pPeer);
    if (!found) found = player_at(world.name, x, y, nullptr);
    if (!found)
    {
        tell(event.peer, "`4Use that on a player.``");
        return true;
    }
    ::peer *t = static_cast<::peer*>(found->data);
    if (t->role > pPeer->role)
    {
        tell(event.peer, "`4That player is immune.``");
        return true;
    }
    use_one();
    const std::string me = pPeer->display_growid, them = t->display_growid;

    switch (id)
    {
        case 126: add_state(*t, fx::HORNS, 3600); refresh(*found); world_msg(*pPeer, std::format("{} `4gave`` {} `4devil horns!``", me, them)); break;
        case 128: add_state(*t, fx::HALO, 3600);  refresh(*found); world_msg(*pPeer, std::format("{} `2gave`` {} `2a golden halo!``", me, them)); break;
        case 196: t->tint = 0xE63014FFu; t->tint_item = 196; t->tint_until = now + 3600; refresh(*found); bubble(*t, "`1*covered in blueberry*``"); break;
        case 338: t->move = ::pos{ 250.0f, -300.0f }; t->move_item = 338; t->move_until = now + 3; refresh(*found); bubble(*t, "Wheee!"); break;
        case 368: t->move = ::pos{ 120.0f, 1000.0f }; t->move_item = 368; t->move_until = now + 10; refresh(*found); bubble(*t, "*splat*"); break;
        case 384: bubble(*t, "`4<3``"); world_msg(*pPeer, std::format("{} `4is now the Valentine of`` {}`4!``", them, me)); break;
        case 388: add_state(*t, fx::SPARKLE, 600); refresh(*found); bubble(*t, "*smells... distinctive*"); break;
        case 614: add_state(*t, fx::STINK, 600); refresh(*found); bubble(*t, "*stinks*"); break;
        case 616: bubble(*t, "`2*hugs the cuddly bunny*``"); world_msg(*pPeer, std::format("{} `2tossed a Cuddly Bunny to`` {}`2!``", me, them)); break;
        case 618:
        {
            ENetEvent ev{};
            ev.peer = found;
            action::respawn(ev, "");
            world_msg(*pPeer, std::format("{}`4's Psychotic Bunny attacked`` {}`4!``", me, them));
            break;
        }
        case 764: add_state(*t, fx::ZOMBIE, 600); refresh(*found); bubble(*t, "`2Braaains...``"); break;
        case 874:
        case 962: bubble(*t, "*SPLAT*"); world_msg(*pPeer, std::format("{} `othrew a`` `w{}`` `oat`` {}`o!``", me, item.raw_name, them)); break;
        case 1368:
        {
            if (!(t->frozen && t->freeze_until == 0))
            {
                t->frozen = true;
                t->state |= S_FROZEN;
                t->freeze_until = now + 10;
                refresh(*found);
                send_varlist(found, { "OnSetFreezeState", 1u }, t->netid);
            }
            bubble(*t, "`1*frozen solid*``");
            break;
        }
    }
    return true;
}

/* runs once a second: ends item effects */
void useitems_tick(std::time_t now)
{
    peers("", peer_condition::PEER_ALL, [now](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;

        bool changed = false;
        const std::size_t before = t->item_states.size();
        std::erase_if(t->item_states, [now](const auto &e) { return e.second <= now; });
        if (t->item_states.size() != before) changed = true;
        if (t->tint_until != 0 && now >= t->tint_until) { t->tint_until = 0; changed = true; }
        if (t->move_until != 0 && now >= t->move_until) { t->move_until = 0; changed = true; }
        if (changed && t->netid != 0) on::SetClothing(p);
    });
}


/* ---------------------------------------------------------- wrench menu */

static std::string left_str(std::time_t ends)
{
    long long s = static_cast<long long>(ends) - static_cast<long long>(std::time(nullptr));
    if (s < 0) s = 0;
    const long long d = s / 86400, h = s % 86400 / 3600, m = s % 3600 / 60, sec = s % 60;
    if (d) return std::format("{} days, {} hours, {} mins, {} secs", d, h, m, sec);
    if (h) return std::format("{} hours, {} mins, {} secs", h, m, sec);
    if (m) return std::format("{} mins, {} secs", m, sec);
    return std::format("{} secs", sec);
}

/* "Effect (x left)" lines for the wrench menu, empty when nothing is active */
std::string item_effects_wrench(const ::peer &p)
{
    const std::time_t now = std::time(nullptr);
    std::string out{};
    auto line = [&](const char *label, int icon, std::time_t ends)
    {
        out += std::format("add_label_with_icon|small|`w{}`` (`w{}`` left)|left|{}|\n", label, left_str(ends), icon);
    };

    for (const auto &[flag, ends] : p.item_states)
    {
        if (ends <= now) continue;
        switch (flag)
        {
            case fx::HORNS:   line("Devil Horns", 126, ends); break;
            case fx::HALO:    line("Golden Halo", 128, ends); break;
            case fx::STINK:   line("Stinky", 614, ends); break;
            case fx::SPARKLE: line("Perfumed", 388, ends); break;
            case fx::ZOMBIE:  line("Infected", 764, ends); break;
            case 0x40000:     line("Haunted", 1988, ends); break;
        }
    }
    if (p.tint_until > now)
        line(p.tint_item == 4766 ? "Cherry Red" : p.tint_item == 540 ? "Green Beer Green" : "Blueberry Blue", p.tint_item ? p.tint_item : 196, p.tint_until);
    if (p.move_until > now)
        line(p.move_item == 338 ? "Floating" : "Muddy", p.move_item ? p.move_item : 368, p.move_until);
    if (p.frozen)
    {
        if (p.freeze_until > now) line("Frozen", 274, p.freeze_until);
        else out += "add_label_with_icon|small|`wFrozen``|left|274|\n";
    }
    return out;
}

/* items with the Double Jump mod, from growtopiawiki.com/w/Mods/Double_Jump (all slots), sorted */
static const std::vector<u_short> double_jump_items{
    156, 362, 678, 736, 818, 820, 1166, 1206, 1460, 1550, 1574, 1672, 1674, 1738, 1780, 1784,
    1824, 1934, 1936, 1938, 1958, 1970, 2158, 2160, 2162, 2164, 2166, 2168, 2254, 2256, 2258, 2260,
    2262, 2264, 2390, 2392, 2438, 2538, 2642, 2722, 2778, 3104, 3112, 3114, 3120, 3134, 3144, 3442,
    3512, 3858, 4184, 4412, 4414, 4534, 4628, 4970, 4972, 4986, 5020, 5322, 5738, 5754, 6004, 6144,
    6284, 6334, 6694, 6758, 6818, 6842, 7084, 7104, 7150, 7196, 7304, 7350, 7414, 7416, 7502, 7582,
    7584, 7648, 7670, 7676, 7678, 7680, 7682, 7696, 7734, 7834, 7910, 7914, 7916, 8024, 8026, 8194,
    8286, 8302, 8308, 8362, 8366, 8552, 8576, 8578, 8580, 8582, 8586, 8588, 8620, 8862, 8914, 9006,
    9008, 9010, 9012, 9014, 9064, 9094, 9114, 9132, 9172, 9182, 9184, 9190, 9210, 9320, 9322, 9344,
    9352, 9394, 9408, 9422, 9428, 9446, 9610, 9656, 9712, 9730, 9732, 9760, 10148, 10168, 10170, 10172,
    10178, 10180, 10182, 10184, 10186, 10188, 10210, 10332, 10338, 10424, 10426, 10496, 10500, 10502, 10534, 10576,
    10632, 10634, 10644, 10666, 10674, 10684, 10754, 10806, 10892, 10952, 10954, 10956, 10958, 10960, 11016, 11048,
    11050, 11134, 11142, 11238, 11292, 11294, 11300, 11302, 11318, 11350, 11352, 11356, 11378, 11462, 11478, 11480,
    11504, 11506, 11508, 11544, 11548, 11552, 11554, 11556, 11558, 11560, 11588, 11660, 11662, 11664, 11666, 11714,
    11724, 11766, 11790, 11792, 11818, 11870, 11906, 11908, 11988, 11992, 12174, 12186, 12224, 12234, 12246, 12248,
    12304, 12306, 12350, 12354, 12376, 12380, 12388, 12390, 12406, 12414, 12418, 12432, 12434, 12630, 12632, 12634,
    12638, 12640, 12646, 12648, 12650, 12842, 12848, 12856, 12864, 12866, 12868, 12872, 12874, 12892, 12996, 13002,
    13022, 13024, 13062, 13068, 13106, 13108, 13112, 13140, 13162, 13196, 13228, 13266, 13268, 13328, 13372, 13396,
    13398, 13408, 13412, 13414, 13418, 13424, 13432, 13434, 13458, 13460, 13500, 13608, 13692, 13714, 13716, 13718,
    13836, 13838, 13840, 13892, 13894, 13896, 13898, 13900, 13936, 13938, 13960, 13968, 14004, 14006, 14028, 14052,
    14068, 14070, 14086, 14146, 14148, 14196, 14232, 14234, 14238, 14266, 14288, 14290, 14390, 14506, 14550, 14552,
    14566, 14596, 14604, 14612, 14632, 14694, 14720, 14722, 14800, 14830, 14836, 14866, 14990, 14992, 15020, 15104,
    15106, 15196, 15232, 15236, 15238, 15280, 15284, 15288, 15290, 15336, 15338, 15380, 15392, 15416, 15460, 15466,
    15522, 15548, 15550, 15596, 15654, 15694, 15696, 15702, 15718, 15726, 15780, 15782, 15798, 15800, 15826, 15840,
    15842, 15850, 15852, 15890, 15934, 15936, 15944, 15946, 15980, 15982, 15984, 15992, 16012, 16048, 16134, 16180,
    16212, 16216, 16218, 16262, 16268, 16270, 16314, 16316, 16362, 16364,
};

/* S_DOUBLE_JUMP if anything worn has the Double Jump mod */
int double_jump_state(const ::peer &p)
{
    if (!p.recent_worlds.empty() && world_jammer(p.recent_worlds.back(), 4992)) return S_DOUBLE_JUMP; // @note Antigravity Generator
    for (float cloth : p.clothing)
        if (std::ranges::binary_search(double_jump_items, static_cast<u_short>(cloth))) return S_DOUBLE_JUMP;
    return 0;
}

/* clothing mods, states and world effects for the wrench menu */
std::string mods_wrench(const ::peer &p)
{
    std::string out{};
    auto line = [&](const std::string &label, int icon)
    {
        out += std::format("add_label_with_icon|small|`w{}``|left|{}|\n", label, icon);
    };

    for (float cloth : p.clothing)
    {
        const u_short id = static_cast<u_short>(cloth);
        if (id == 0) continue;
        const std::string &name = id_to_item(id).raw_name;
        if (std::ranges::binary_search(double_jump_items, id)) line(std::format("Double Jump: {}", name), id);
        if (get_punch_id(id) != 0)                            line(std::format("Punch Effect: {}", name), id);
        if (id == 12412)                                      line("Punch Range: Wizard's Wand", id);
        if (id == 1956)                                       line("Punch Damage: Chaos Cursed Wand", id);
    }

    if (p.god_mode)                line("God Mode: nothing can hurt you", 18);
    if (p.state & S_GHOST)         line("Ghost: walk through anything", 18);
    if (p.state & S_DUCT_TAPE)     line("Duct Tape: mmmfff!", 408);
    if (auto w = std::ranges::find(worlds, p.recent_worlds.back(), &::world::name); w != worlds.end() && w->base_weather == 7)
        line("Low Gravity: Mars", 1136);
    return out;
}