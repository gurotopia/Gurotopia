#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "tools/random.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "buffs_items.hpp"

struct buff_def
{
    u_short item;
    u_char id;
    const char *name;
    int seconds;
    bool food; // @note says YUM!, and Glutton's Gazpacho makes it last longer
};

static const std::vector<buff_def> buff_items{
    { 1474,  buff::XP_DOUBLE,  "Double XP Chance",    1800, true  }, // Eggs Benedict
    { 5452,  buff::XP_TRIPLE,  "Triple XP Chance",    1800, true  }, // Gingerbread Cookie
    { 4324,  buff::XP_BOOST,   "Brain Enhanced",      1800, false }, // Biotronic Brain Enhancer
    { 4768,  buff::CONSUME_XP, "Sticky Fingers",      1800, true  }, // Pineapple Turnover
    { 4604,  buff::GEM_CHANCE, "Gem Finder",          1800, true  }, // Arroz Con Pollo
    { 4348,  buff::GEM_DOUBLE, "Gemmin'",             1800, false }, // Gemmin' Juice
    { 750,   buff::LUCKY,      "Lucky",               1800, false }, // Rabbit's Foot
    { 528,   buff::LUCKY,      "Lucky",               1800, false }, // Lucky Clover
    { 4594,  buff::TREE_GROW,  "Green Thumb",         1800, true  }, // Apple Strudel
    { 5984,  buff::HIGH_JUMP,  "High Jump",           1800, true  }, // Chocolate Bunny
    { 12152, buff::SPEEDY,     "Speedy",              600,  false }, // Janeway's Coffee
    { 5262,  buff::SPEEDY,     "Speedy",              600,  false }, // Neon Gum
    { 12156, buff::PUNCH,      "Punch Damage",        600,  true  }, // Blood Truffle Biscuits
    { 1662,  buff::SPIKE,      "Spike Proof",          5,    false }, // Spike Juice
    { 6908,  buff::GAZPACHO,   "Glutton",             1800, true  }, // Glutton's Gazpacho
    { 10660, buff::PURE_LOVE,  "Pure Love",           1800, false }, // Pure Love Essence
    { 1056,  buff::LUCKY,      "Lucky",               1800, true  }, // Songpyeon
};

static const char *name_of(u_char id)
{
    for (const buff_def &d : buff_items) if (d.id == id) return d.name;
    return "buff";
}

static std::string left_str(std::time_t ends)
{
    long long s = static_cast<long long>(ends) - static_cast<long long>(std::time(nullptr));
    if (s < 0) s = 0;
    const long long h = s / 3600, m = s % 3600 / 60, sec = s % 60;
    if (h) return std::format("{} hours, {} mins, {} secs", h, m, sec);
    if (m) return std::format("{} mins, {} secs", m, sec);
    return std::format("{} secs", sec);
}

bool buff_active(const ::peer &p, u_char id)
{
    const std::time_t now = std::time(nullptr);
    return std::ranges::any_of(p.buffs, [&](const ::active_buff &b) { return b.id == id && b.ends > now; });
}

u_short buff_xp(const ::peer &p, u_short value)
{
    long long v = value;
    if (buff_active(p, buff::XP_BOOST)) { v += v / 2; if ((value % 2) && RandomRange(0, 2)) v += 1; }
    if (buff_active(p, buff::XP_DOUBLE) && !RandomRange(0, 4))  v *= 2;
    if (buff_active(p, buff::XP_TRIPLE) && !RandomRange(0, 10)) v *= 3;
    return static_cast<u_short>(std::min<long long>(v, 65535));
}

u_short consumable_xp(const ::peer &p)
{
    return buff_active(p, buff::CONSUME_XP) ? 3 : 1;
}

bool buff_use(ENetEvent &event, const ::item &item)
{
    auto def = std::ranges::find(buff_items, item.id, &buff_def::item);
    if (def == buff_items.end()) return false;
    ::peer *p = static_cast<::peer*>(event.peer->data);

    int secs = def->seconds;
    if (def->food && def->id != buff::GAZPACHO && buff_active(*p, buff::GAZPACHO)) secs = secs * 13 / 10;
    const std::time_t ends = std::time(nullptr) + secs;

    bool found = false;
    for (::active_buff &b : p->buffs) if (b.id == def->id) { b.ends = ends; b.item = item.id; found = true; }
    if (!found) p->buffs.push_back({ def->id, ends, item.id });

    modify_item_inventory(event, ::slot(static_cast<short>(item.id), -1));
    p->add_xp(event, consumable_xp(*p));

    if (def->food)
        peers(p->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &x) { send_varlist(&x, { "OnTalkBubble", speaker(*p).netid, "YUM!", 0u }); });
    on::ConsoleMessage(event.peer, std::format("`2{}`` `ois active for`` `w{}``.", def->name, left_str(ends)));

    if (def->id == buff::SPEEDY || def->id == buff::HIGH_JUMP) on::SetClothing(*event.peer);
    return true;
}

void buff_on_break(ENetEvent &event, ::world &world, const ::item &item, const ::gamePacket &gamePacket)
{
    if (item.type == type::SEED) return;
    ::peer *p = static_cast<::peer*>(event.peer->data);
    const ::pos at = gamePacket.punch.by_32();

    if (buff_active(*p, buff::GEM_CHANCE) && !RandomRange(0, 10)) add_drop(event, ::slot(112, 1), at, world);
    if (buff_active(*p, buff::GEM_DOUBLE) && !RandomRange(0, 10)) add_drop(event, ::slot(112, static_cast<short>(RandomRange(1, 6))), at, world);
    if (buff_active(*p, buff::LUCKY)      && !RandomRange(0, 10)) add_drop(event, ::slot(static_cast<short>(item.id + 1), 1), at, world);
}

u_int buff_tree_head_start(const ::peer &p, const ::item &seed)
{
    return buff_active(p, buff::TREE_GROW) ? static_cast<u_int>(seed.tick) / 20 : 0; // @note 5%
}

std::string buffs_wrench(const ::peer &p)
{
    const std::time_t now = std::time(nullptr);
    std::string out{};
    for (const ::active_buff &b : p.buffs)
        if (b.ends > now)
            out += std::format("add_label_with_icon|small|`w{}`` (`w{}`` left)|left|{}|\n", name_of(b.id), left_str(b.ends), b.item);
    return out;
}

/* runs once a second: ends buffs */
void buffs_tick(std::time_t now)
{
    peers("", peer_condition::PEER_ALL, [now](ENetPeer &x)
    {
        ::peer *t = static_cast<::peer*>(x.data);
        if (!t || t->growid.empty()) return;

        bool refresh = false;
        std::erase_if(t->buffs, [&](const ::active_buff &b)
        {
            if (b.ends > now) return false;
            if (b.id == buff::SPEEDY || b.id == buff::HIGH_JUMP) refresh = true;
            on::ConsoleMessage(&x, std::format("`oYour `w{}`` `owore off.``", name_of(b.id)));
            return true;
        });
        if (refresh && t->netid != 0) on::SetClothing(x);
    });
}
