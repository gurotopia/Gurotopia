#include "pch.hpp"
#include <map>
#include "onVariant/ConsoleMessage.hpp"
#include "tools/time.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "cooking.hpp"

std::vector<::cooking_pot> cooking_pots{};

/* ------------------------------------------------------------ recipe data
 * written by item NAME - ids are resolved from items.dat on first use.
 * names must match the in-game name exactly (singular, not plural).
 */
struct named_recipe
{
    std::vector<std::pair<const char*, int>> needs; // @note {item name, amount}
    const char *makes;
};

static const std::vector<::named_recipe> named_recipes = {
    /* ---- cooking ---- */
    { { {"Wheat", 4}, {"Egg", 1}, {"Sugar Cane", 1} },                         "Dough" },
    { { {"Egg", 1}, {"Milk", 1}, {"Dough", 4} },                               "Bread" },
    { { {"Dough", 3}, {"Swiss Cheese Block", 1}, {"Tomato", 1} },              "Pizza" },
    { { {"Potato", 1}, {"Fish Chunk", 2}, {"Wheat", 3} },                      "Fish And Chips" },

    /* ---- general ---- */
    { { {"Cyclopean Visor", 10}, {"Checkpoint", 1}, {"Portcullis", 1} },       "Laser Grid" },
    { { {"Stone Egg", 1}, {"Lava", 200}, {"Boss Goo", 1} },                    "Ghost Egg" },
    { { {"Stone Egg", 1}, {"Lava", 200}, {"Wriggling Tentacle", 1} },          "Void Egg" },
    { { {"Stone Egg", 1}, {"Lava", 200}, {"Head Bangin' Steel Wolf", 1} },     "Water Egg" },
    { { {"Rune Stone", 20}, {"Dragon Glass", 10}, {"Table Lamp", 1} },         "Dragon Lamp" },
    { { {"Rune Stone", 20}, {"Dragon Glass", 10}, {"Bathroom Mirror", 1} },    "Dragon Mirror" },
    { { {"Rune Stone", 20}, {"Dragon Glass", 10}, {"Theater Seat", 10} },      "Dragon Throne" },
    { { {"U.S.S. Protostar Bulkhead", 200}, {"Protostar Captain's Seat", 10}, {"Steel Block", 200} }, "Vehicle Replicator" },

    /* ---- anniversary ---- */
    { { {"80's Tape Deck", 1}, {"Mega Rock Speaker", 2}, {"D Battery", 4} },   "Party Blaster" },
    { { {"Reanimator Remote", 1}, {"Spare Body", 1}, {"Limb Connector", 200} },"Living Dead Remote" },
    { { {"Party Style Scarf", 1}, {"Party Bunting", 1}, {"Transmog Crystal", 1} }, "Ultra Party Glow Scarf" },
    { { {"Party Style Top Hat", 1}, {"Party Hat", 1}, {"Glowstick", 1} },      "Ultra Party Glow Top Hat" },
    { { {"Party Style Shades", 1}, {"Party Glasses", 1}, {"Transmog Crystal", 1} }, "Ultra Party Glow Shades" },
    { { {"Party Light Wave Vest", 1}, {"Party Vest", 1}, {"Transmog Crystal", 1} }, "Ultra Party Light Wave Vest" },
    { { {"Equalizer Tee", 1}, {"Party Tunes", 10}, {"Tempered Party Tunes", 5} }, "Ultra Equalizer Tee" },
    { { {"Mega Rock Speaker", 3}, {"Party Tunes", 10}, {"Tempered Party Tunes", 5} }, "Megawatt Bass Speaker" },
    { { {"Stone Egg", 1}, {"Raver Hoodie", 1}, {"Rave Stage", 20} },           "Euphoric Dragon Egg" },
    { { {"Das Red Balloon", 5}, {"Hero Portrait - Buddy", 1}, {"Sand", 1} },   "Buddy Balloons" },

    /* ---- forged ---- */
    { { {"Iron Buckle", 1}, {"Silkworm Leash", 1}, {"Burnt Leather", 4} },     "Silkworm Saddle" },
    { { {"Gift of the Unicorn", 1}, {"Sword Blade", 2}, {"Sword Pommel", 2} }, "Twin Swords" },
    { { {"Warrior's Shield", 1}, {"Sword Blade", 1}, {"Sword Pommel", 1} },    "Sword And Shield" },
    { { {"Grip Tape", 2}, {"Steel Axe Handle", 2}, {"Pick Blade", 2} },        "Climbing Picks" },
    { { {"Cosmic Bridle", 1}, {"Pegasus", 1}, {"Synthetic Chemical", 20} },    "Cosmic Sky Horse" },
    { { {"Celestial Blade", 1}, {"Grip Tape", 1}, {"Steel Axe Handle", 2} },   "Celestial Lance" },

    /* ---- miscellaneous ---- */
    { { {"Transmog Crystal", 1}, {"Lava", 1}, {"Super Crate Box", 20} },       "Lava Cube" },
    { { {"Transmog Crystal", 1}, {"Cow", 1}, {"Brown Block", 20} },            "Buffalo" },
    { { {"Transmog Crystal", 1}, {"Death Spikes", 1}, {"Steam Tubes", 20} },    "Steam Spikes" },
    { { {"Doodad", 1}, {"Doohickey", 1}, {"Whatchamacallit", 1} },             "Legen Seed" },
    { { {"Kerjigger", 1}, {"Thingamajig", 1}, {"Thingamabob", 1} },            "Dary Seed" },
    { { {"Transmog Crystal", 1}, {"Paintbrush", 1}, {"Art Wall", 20} },        "Painting Easel" },
    { { {"Transmog Crystal", 1}, {"Diamond Lock", 1}, {"Robot Wants Dubstep", 20} }, "Robotic Lock" },
    { { {"Unearthly Chemical", 10}, {"Synthetic Chemical", 10}, {"Mysterious Chemical", 10} }, "Untrade-a-Box" },
    { { {"Unearthly Chemical", 10}, {"Radioactive Chemical", 10}, {"Synthetic Chemical", 10} }, "Tomb Robber" },
    { { {"Transmog Crystal", 1}, {"Challenge Timer", 1}, {"Race Start Flag", 20} }, "Challenge Start Flag" },
    { { {"Transmog Crystal", 1}, {"Challenge Timer", 1}, {"Race End Flag", 20} },   "Challenge End Flag" },
    { { {"Radioactive Chemical", 200}, {"Haunted Chemical", 200}, {"Unearthly Chemical", 200} }, "Xenonite Crystal" },
    { { {"Mysterious Chemical", 1}, {"Radioactive Chemical", 5}, {"Haunted Chemical", 50} }, "Containment Field Power Node" },

    /* ---- lunar new year ---- */
    { { {"Terracotta Pot", 100}, {"Red Coin Wallpaper", 5}, {"Statue Block", 50} }, "Terracotta Growtopian" },
    { { {"Chinese Temple Pillar", 200}, {"Grip Tape", 1}, {"Lucky Golden Crest", 2} }, "Monkey Warrior's Staff" },
    { { {"Bamboo", 1}, {"Checker Wallpaper", 10}, {"Milk", 1} },               "Bamboo Boba Block" },
    { { {"Bunny Ears", 5}, {"Jade Pillar", 6}, {"Paper Dividing Wall", 5} },   "Zodiac Rabbit Ears" },
    { { {"Terracotta Couch", 1}, {"Lunar Spice Spray", 1}, {"Paper Dividing Wall", 1} }, "Ta Couch" },

    /* ---- valentine's ---- */
    { { {"Lovewillow's Lace", 5}, {"Table Lamp", 1}, {"Golden Block", 1} },    "Lovely Dinner Light" },
    { { {"Charmer Smile", 1}, {"Muscle Suit", 1}, {"Steel Block", 200} },      "Beefy Biceps" },
    { { {"Medical Scarf", 1}, {"Mutated Virus", 1}, {"Grey Cat As A Hat", 1} },"Snuggly Kitty Scarf" },
    { { {"Heart Antennae Headband", 1}, {"Transmog Crystal", 1}, {"Candy Heart", 10} }, "Heart Antennae Hat" },
    { { {"Candy Heart", 10}, {"Cotton Candy Clouds", 1}, {"Valentine's Dust", 10} },    "Tunnel of Love" },
    { { {"Cotton Candy Clouds", 50}, {"Polka Dot Block", 50}, {"Strawberry Block", 1} }, "Strawbeanie" },
    { { {"Colorful Cake Slice", 1}, {"Cotton Candy Clouds", 1}, {"Carnival Platform", 1} }, "Rainbow Platform" },
    { { {"Golden Heart Crystal", 1}, {"Heart Glasses", 1}, {"Candy Heart", 200} },   "Golden Heart Glasses" },
    { { {"Golden Heart Crystal", 1}, {"Ruby Necklace", 1}, {"Candy Heart", 200} },   "Golden Diamond Necklace" },
    { { {"Golden Heart Crystal", 1}, {"Heartbow", 1}, {"Candy Heart", 200} },        "Golden Heartbow" },
    { { {"Golden Heart Crystal", 1}, {"Angel Wings", 1}, {"Candy Heart", 200} },     "Golden Angel Wings" },
    { { {"Golden Heart Crystal", 1}, {"Diaper", 1}, {"Candy Heart", 200} },          "Golden Diaper" },
    { { {"Golden Heart Crystal", 1}, {"Love Bug", 1}, {"Candy Heart", 200} },        "Golden Love Bug" },
    { { {"Golden Heart Crystal", 1}, {"Pegasus", 1}, {"Candy Heart", 200} },         "Golden Pegasus" },
    { { {"Golden Heart Crystal", 1}, {"Air Robinsons", 1}, {"Candy Heart", 200} },   "Golden Air Robinsons" },
    { { {"Golden Heart Crystal", 1}, {"Heartbreaker Hammer", 1}, {"Candy Heart", 200} }, "Heavenly Scythe" },
    { { {"Golden Heart Crystal", 1}, {"Teeny Angel Wings", 1}, {"Candy Heart", 200} },   "Teeny Golden Wings" },
    { { {"Golden Heart Crystal", 1}, {"Heart Shirt", 1}, {"Candy Heart", 200} },     "Golden Heart Shirt" },
    { { {"Golden Heart Crystal", 1}, {"USMom's Talaria", 1}, {"Candy Heart", 200} },         "Golden Talaria" },
    { { {"Golden Heart Crystal", 1}, {"Heartstaff", 1}, {"Candy Heart", 200} },      "Golden Heartstaff" },
    { { {"Golden Heart Crystal", 1}, {"Heartsword", 1}, {"Candy Heart", 200} },      "Golden Heartsword" },
    { { {"Golden Heart Crystal", 1}, {"Heartthrob Helm", 1}, {"Candy Heart", 200} }, "Golden Heartthrob Helm" },
    { { {"Golden Heart Crystal", 1}, {"Cloud Nine Cape", 1}, {"Candy Heart", 200} }, "Golden Sunset Cape" },
    { { {"Golden Heart Crystal", 1}, {"Datemaster's Bling", 1}, {"Datemaster's Rose", 1} }, "Datemaster's Heart Locket" },
    { { {"Golden Heart Crystal", 1}, {"Heartstaff", 1}, {"Heartsword", 1} },         "Stained Glass Heartwings" },
    { { {"Golden Heart Crystal", 1}, {"Silk Scarf", 1}, {"Candy Heart", 200} },      "Golden Silk Scarf" },
    { { {"Golden Heart Crystal", 1}, {"Loving Halo", 1}, {"Candy Heart", 200} },     "Golden Heart Aura" },
    { { {"Golden Heart Crystal", 1}, {"Heartbreak Wings", 1}, {"Candy Heart", 200} },"Golden Heartbreak Wings" },
    { { {"Golden Heart Crystal", 1}, {"Stained Glass Scepter", 1}, {"Candy Heart", 200} }, "Golden Stained Glass Scepter" },
    { { {"Golden Heart Crystal", 1}, {"Ribbon Wings", 1}, {"Candy Heart", 200} },    "Golden Ribbon Wings" },

    /* ---- st. patrick's ---- */
    { { {"Cobra Statuette", 1}, {"Yellow Diamond", 2}, {"Lucky Clover", 10} },  "Serpent Staff" },
    { { {"Frosty Wings", 1}, {"Emerald Shard", 10}, {"Lucky Clover", 15} },     "Shamrock Wings" },
    { { {"Emerald Shard", 2}, {"Lazy Cobra", 10}, {"Potato Couch", 2} },        "King Cobra Chair" },
    { { {"Cloverleaf", 1}, {"Bouncy Balloon", 10}, {"Moss-Covered Stone", 5} }, "Shamrock Bouncing Buddy" },
    { { {"Lucky Clover", 1}, {"Green Beer", 10}, {"Fountain", 5} },             "Green Fountain" },
    { { {"Leprechaun Hat", 1}, {"Green Pants", 1}, {"Lucky Clover", 5} },       "Golden Leprechaun Hat" },
    { { {"Leprechaun Suit", 1}, {"Leprechaun Shoes", 1}, {"Lucky Clover", 5} }, "Golden Leprechaun Suit" },
    { { {"Field Grass", 5}, {"Window Curtains", 50}, {"Celtic Block", 10} },    "Green Curtains" },
    { { {"Green Beer", 5}, {"Gingerbread Roof", 25}, {"Stone Wall", 50} },      "Ye Olde Tavern Roof" },
    { { {"Round Shield", 1}, {"Lucky Clover", 20}, {"Leprechaun Shoes", 100} }, "Shamrock Shields" },

    /* ---- super pineapple party ---- */
    { { {"Super Pineapple", 1}, {"Mysterious Chemical", 1}, {"Uranium Block", 20} }, "Super Exploding Pineapple" },
    { { {"Pineapple Engine Part", 2}, {"Pineapple Wheel", 4}, {"Pineapple Plating", 10} }, "Pine-ch Buggy" },
    { { {"Super Pineapple", 3}, {"Dangerous Pineapple", 3}, {"Pineapple", 100} },    "Pineapple Guzzler" },
    { { {"High Tech Block", 5}, {"Pineapple", 5}, {"Dangerous Pineapple", 1} },      "Super Pineapple Time Warp Device" },
    { { {"Pineapple Pizza Slice", 50}, {"Pineapple Root Cutting", 200}, {"Pineapple Spear", 1} }, "Giant Pineapple Pizza Paddle" },
};

/* ------------------------------------------------------ resolved recipes */

struct recipe
{
    std::vector<std::pair<short, int>> needs; // @note {item id, amount}
    short makes;
    std::string name;
};

static std::vector<::recipe> recipes{};
static bool resolved = false;

/* case-insensitive exact name match against items.dat */
static short id_from_name(const char *name)
{
    std::string want = name;
    for (char &c : want) c = std::tolower(static_cast<unsigned char>(c));

    for (const ::item &it : items)
    {
        if (it.raw_name.size() != want.size()) continue;
        std::string have = it.raw_name;
        for (char &c : have) c = std::tolower(static_cast<unsigned char>(c));
        if (have == want) return static_cast<short>(it.id);
    }
    return 0;
}

static void resolve_recipes()
{
    if (resolved) return;
    resolved = true;

    std::size_t ok = 0, skipped = 0;

    for (const ::named_recipe &nr : named_recipes)
    {
        ::recipe r{};
        r.name = nr.makes;
        r.makes = id_from_name(nr.makes);

        bool complete = (r.makes != 0);
        if (!complete) printf("[combiner] unknown result '%s'\n", nr.makes);

        for (const auto &[name, amount] : nr.needs)
        {
            const short id = id_from_name(name);
            if (id == 0)
            {
                printf("[combiner] '%s' needs unknown ingredient '%s'\n", nr.makes, name);
                complete = false;
            }
            r.needs.emplace_back(id, amount);
        }

        if (complete) { recipes.emplace_back(std::move(r)); ++ok; }
        else ++skipped;
    }

    printf("[combiner] %zu recipes loaded, %zu skipped (names not found)\n", ok, skipped);
}

/* ---------------------------------------------------------------- helpers */

static bool on_tile(const ::object &o, int x, int y)
{
    return o.count > 0
        && static_cast<int>(o.pos.x / 32.0f) == x
        && static_cast<int>(o.pos.y / 32.0f) == y;
}

static std::map<short, int> tally_tile(const ::world &world, int x, int y)
{
    std::map<short, int> have{};
    for (const ::object &o : world.objects)
        if (on_tile(o, x, y)) have[static_cast<short>(o.id)] += o.count;
    return have;
}

/* the recipe using the most distinct ingredients wins */
static const ::recipe *find_recipe(const std::map<short, int> &have)
{
    const ::recipe *best = nullptr;
    std::size_t best_size = 0;

    for (const ::recipe &r : recipes)
    {
        bool ok = true;
        for (const auto &[id, need] : r.needs)
        {
            auto it = have.find(id);
            if (it == have.end() || it->second < need) { ok = false; break; }
        }
        if (ok && r.needs.size() >= best_size) { best = &r; best_size = r.needs.size(); }
    }
    return best;
}

/* remove a dropped object without crediting anyone - netid -1 means it just vanishes */
static void vanish_object(ENetEvent& event, ::world &world, u_int uid)
{
    item_change_object(event, ::gamePacket{
        .netid = 0, // @note no player has netid 0: removed, collected by nobody
        .uid   = (int)0xffffffff,
        .id    = static_cast<int>(uid)
    });
    std::erase_if(world.objects, [uid](const ::object &o) { return o.uid == uid; });
}

/* set the oven open or closed visually.
 * if it ever looks backwards in-game, swap the two lines inside the if/else */
static void set_oven(ENetEvent& event, ::world &world, int x, int y, bool closed)
{
    ::block &b = world.blocks[cord(x, y)];
    if (closed) b.state[2] &= ~S_TOGGLE;
    else        b.state[2] |= S_TOGGLE;
    send_tile_update(event, { .id = b.fg, .punch = ::pos{ x, y } }, b, world);
}

/* ------------------------------------------------------------------ usage */

bool cooking_use(ENetEvent& event, ::world &world, int x, int y)
{
    resolve_recipes();

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* a finished dish is waiting inside - this punch takes it out */
    for (auto it = cooking_pots.begin(); it != cooking_pots.end(); ++it)
    {
        if (it->world != world.name || it->x != x || it->y != y) continue;

        const short made = it->result;
        cooking_pots.erase(it);

        set_oven(event, world, x, y, false); // @note open it up
        add_drop(event, ::slot(made, 1), ::pos{ x * 32.0f + 8.0f, (y - 1) * 32.0f + 8.0f }, world);

        const ::item &dish = id_to_item(made);
        on::ConsoleMessage(event.peer, std::format("`2Took out a `w{}``!``", dish.raw_name));
        return true;
    }

    const std::map<short, int> have = tally_tile(world, x, y);

    /* empty: let the punch through so it can be broken */
    if (have.empty()) return false;

    std::string listing{};
    for (const auto &[id, count] : have)
    {
        const ::item &ing = id_to_item(id);
        if (!listing.empty()) listing += "`o, ";
        listing += std::format("`w{}`` x`w{}``", ing.raw_name, count);
    }
    on::ConsoleMessage(event.peer, std::format("`oIn the combiner: {}", listing));

    const ::recipe *r = find_recipe(have);

    if (r == nullptr)
    {
        on::ConsoleMessage(event.peer, "`4Nothing can be made from that.``");
        return true;
    }

    /* decide what to consume before touching the object list */
    std::map<short, int> still_need{};
    for (const auto &[id, need] : r->needs) still_need[id] = need;

    std::vector<u_int> remove_uids{};
    std::vector<std::pair<u_int, int>> shrink{};

    for (const ::object &o : world.objects)
    {
        if (!on_tile(o, x, y)) continue;

        auto it = still_need.find(static_cast<short>(o.id));
        if (it == still_need.end() || it->second <= 0) continue;

        if (o.count <= it->second)
        {
            it->second -= o.count;
            remove_uids.emplace_back(o.uid);
        }
        else
        {
            shrink.emplace_back(o.uid, it->second);
            it->second = 0;
        }
    }

    std::vector<std::pair<::slot, ::pos>> leftovers{};
    for (const auto &[uid, take] : shrink)
        for (const ::object &o : world.objects)
            if (o.uid == uid)
            {
                leftovers.emplace_back(::slot(static_cast<short>(o.id), static_cast<short>(o.count - take)), o.pos);
                remove_uids.emplace_back(uid);
                break;
            }

    for (u_int uid : remove_uids) vanish_object(event, world, uid);
    for (const auto &[s, p] : leftovers) add_drop(event, s, p, world);

    /* the result lands on the tile ABOVE the oven so you can walk into it,
       and so it isn't counted as an ingredient next time */
    ::cooking_pot waiting{};
    waiting.world = world.name;
    waiting.x = x;
    waiting.y = y;
    waiting.owner = pPeer->user_id;
    waiting.result = r->makes;
    cooking_pots.emplace_back(waiting);

    set_oven(event, world, x, y, true); // @note closed while it cooks

    on::ConsoleMessage(event.peer, std::format("`2Made a `w{}``! Punch the oven again to take it out.``", r->name));

    peers(world.name, PEER_SAME_WORLD, [pPeer, r](ENetPeer &p)
    {
        send_varlist(&p, { "OnTalkBubble", pPeer->netid,
            std::format("`5<{} combines a {}>``", pPeer->display_growid, r->name), 1u });
    });

    return true;
}

void cooking_dialog(ENetEvent& event, const ::hPipe &hPipe)
{
}
