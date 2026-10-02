#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "tools/random.hpp"
#include "onVariant/SetBux.hpp"
#include "events.hpp"

static constexpr int EVENT_CHANCE = 3; // @note % chance per player per minute that a world gets a random event (max 50%)

struct event_def
{
    const char *title;
    const char *text;
    std::vector<const char*> items; // @note item names, resolved from items.dat
    int count;                      // @note how many of each item (or in total, when mixed)
    int seconds;                    // @note 0 = no time limit, leftovers stay
    bool gems;                      // @note spawn piles of gems
    bool mixed;                     // @note count drops, each a random item from the list
    bool anytime;                   // @note can start randomly; false = only with /event (holiday events)
};

static const std::vector<event_def> defs{
    /* ---- can happen anytime ---- */
    { "Beautiful Crystal!",   "You have `w30`` seconds to find and grab the `#Crystal Block Seed``.",                   { "Crystal Block Seed" }, 1, 30, false, false, true },
    { "Magical Seeds!",       "You have `w30`` seconds to find and grab the `w4`` `#Mystery Block Seeds``.",            { "Mystery Block Seed" }, 4, 30, false, false, true },
    { "Spoiler Alert!",       "You have `w30`` seconds to find the `w10`` pieces of the `#Secret Of Growtopia``!",      { "Secret Of Growtopia" }, 10, 30, false, false, true },
    { "Desert Blast!",        "You have `w30`` seconds to find and grab the `#Desert Blast``.",                         { "Desert Blast" }, 1, 30, false, false, true },
    { "Well, Well, Well!",    "You have `w30`` seconds to find the `w3`` `#Well`` blocks hidden around the world!",     { "Well" }, 3, 30, false, false, true },
    { "Lost Gold!",           "You have `w30`` seconds to find the `#Gold Choker`` hidden in the world!",               { "Gold Choker" }, 1, 30, false, false, true },
    { "Howling At The Moon!", "You have `w30`` seconds to find `w3`` `#Wolf Whistles`` hidden in the world!",           { "Wolf Whistle" }, 3, 30, false, false, true },
    { "Jungle Blast!",        "You have `w30`` seconds to find `w1`` `#Jungle Blast``.",                                { "Jungle Blast" }, 1, 30, false, false, true },
    { "What's that?",         "You have `w10`` seconds to find and grab the `#Doohickey``.",                            { "Doohickey" }, 1, 10, false, false, true },
    { "That's Puzzling...",   "You have `w20`` seconds to find the `w20`` `#Jigsaw Wallpapers`` hidden in this world!", { "Jigsaw Wallpaper" }, 20, 20, false, false, true },
    { "Jackpot!",             "`#Gems`` are spawned throughout the world!",                                             { }, 0, 0, true, false, true },
    { "Beat The Heat!",       "Cool off with some free `#Water Buckets``!",                                             { "Water Bucket" }, 10, 0, false, false, true },
    { "Chemical Spill!",      "`#Toxic Waste`` spawns everywhere!",                                                      { "Toxic Waste" }, 20, 0, false, false, true },

    /* ---- holiday events: only with /event ---- */
    { "Egg Hunt!",            "Go find them eggs! `w20`` `#Magic Eggs`` have spawned in the world!",                    { "Magic Egg" }, 20, 0, false, false, false },
    { "Valentine's Card!",    "You have `w30`` seconds to collect the `w5`` `#Valentine's Dust`` spawned in your world!", { "Valentine's Dust" }, 5, 30, false, false, false },
    { "Luck of the Growish!", "`#Green Beer`` and `#Lucky Clovers`` spawn everywhere!",                                 { "Green Beer", "Lucky Clover" }, 5, 0, false, false, false },
    { "Lucky Kitty!",         "You have `w20`` seconds to find a `#Lucky Kitty`` or an `#Unlucky Kitty``!",             { "Lucky Kitty", "Unlucky Kitty" }, 1, 20, false, true, false },
    { "Songpyeon!",           "`w3`` `#Songpyeon`` spawned in your world, you have `w30`` seconds to collect them!",    { "Songpyeon" }, 3, 30, false, false, false },
    { "Mooncake Madness!",    "You have `w30`` seconds to find all the `w40`` `#Mooncakes`` scattered around the world!", { "Mooncake" }, 40, 30, false, false, false },
    { "Let it Snow!",         "You have to collect `w30`` `#Snowballs`` before they melt!",                             { "Snowball" }, 30, 30, false, false, false },
    { "Spirit of Giving!",    "`#Winter Gifts`` are spawned everywhere in the world!",                                  { "Winter Gift" }, 10, 0, false, false, false },
    { "Festive Spirit!",      "You have `w30`` seconds to find all `w5`` `#Winterfest Crackers`` hidden in the world!", { "Winterfest Cracker" }, 5, 30, false, false, false },
    { "Royal Winter!",        "`#Royal Winter Seals`` for everyone! Be quick, you have `w30`` seconds to collect them!", { "Royal Winter Seal" }, 5, 30, false, false, false },
    { "Surgical Supply Drop!", "You have `w30`` seconds to find the `w20`` assorted surgical supplies hidden in the world!",
        { "Surgical Sponge", "Surgical Scalpel", "Surgical Anesthetic", "Surgical Antiseptic", "Surgical Antibiotics", "Surgical Splint",
          "Surgical Stitches", "Surgical Pins", "Surgical Transfusion", "Surgical Defibrillator", "Surgical Clamp", "Surgical Ultrasound", "Surgical Lab Kit" },
        20, 30, false, true, false },
    { "Summertime Surprise!", "A `#Summer Surprise`` has spawned!",                                                      { "Summer Surprise" }, 1, 0, false, false, false },
    { "Explodiversary!",      "`#Anniversary Skyrockets`` for everyone! Be quick, you have `w20`` seconds to collect them!", { "Anniversary Skyrocket" }, 5, 20, false, false, false },
    { "Anniversary Party!",   "You have `w30`` seconds to collect the `w5`` party items spawned in your world!",
        { "Ballroom Party Chair", "Birthday Band Hair", "Birthday Cake Hat", "Black Display Shelf", "Confetti Crown", "Confetti Popper",
          "Disco Dude's Pants", "Disco Dude's Shoes", "Electric Guitar", "Equalizer Tee", "Fireflies In A Jar", "Flame Tee", "Fox Ears",
          "Fox Onesie", "Glitter Pillow Trampoline", "Light Box - Green", "Light Box - Pink", "Light Box - Yellow", "Party Block",
          "Party Bunting", "Party Cake", "Party Confetti", "Party Couch", "Party Glasses", "Party Glow Sneakers", "Party Hat", "Party Horn",
          "Party Light Wave Vest", "Party Mocktail Table", "Party Pants", "Party Popper", "Party Screamer", "Party Socks",
          "Party Style Scarf", "Party Style Shades", "Party Style Top Hat", "Party Table", "Party Tunes", "Party Vest",
          "Party-In-A-Box Head", "Rave Cap", "Rave Hall Floor", "Rave Hall Wall", "Rave Haze Light", "Rave Stage", "Raver Hoodie",
          "Raver Pants", "Resting Headphones", "Retro Pattern Background", "Retro Pattern Block", "Roobux Cube",
          "Sheet Music: Electric Guitar", "Skyrocket", "T-Shirt Cannon" },
        5, 30, false, true, false },
};

struct active_event
{
    std::string world;
    std::string title;
    std::time_t ends;
    std::vector<u_int> uids;
};
static std::vector<active_event> active{};

static std::string lower(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static short item_id(const char *name)
{
    const std::string want = lower(name);
    for (const ::item &it : items)
        if (it.raw_name.size() == want.size() && lower(it.raw_name) == want) return static_cast<short>(it.id);
    return 0;
}

static ENetPeer *any_peer_in(const std::string &world)
{
    ENetPeer *found = nullptr;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p) { if (!found) found = &p; });
    return found;
}

static void world_msg(const std::string &world, const std::string &text)
{
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p) { on::ConsoleMessage(&p, text); });
}

/* the "Special Event!" banner + chat line for everyone in the world */
static void notify(const std::string &world, const std::string &text)
{
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &peer)
    {
        send_varlist(&peer, { "OnAddNotification", "interface/large/special_event.rttex", text, "audio/cumbia_horns.wav", 0u });
        on::ConsoleMessage(&peer, text);
    });
}
static bool random_air(::world &w, int &x, int &y)
{
    const int width = static_cast<int>(w.blocks.size() / 60), height = 60;
    for (int tries = 0; tries < 300; ++tries)
    {
        x = RandomRange(0, width);
        y = RandomRange(0, height);
        if (w.blocks[cord(x, y)].fg == 0) return true;
    }
    return false;
}

static bool event_running(const std::string &world)
{
    return std::ranges::any_of(active, [&](const active_event &a) { return a.world == world; });
}

static bool start_event(::world &w, const event_def &d)
{
    ENetPeer *p = any_peer_in(w.name);
    if (!p) return false;
    ENetEvent ev{};
    ev.peer = p;

    std::vector<short> ids{};
    for (const char *name : d.items)
    {
        const short id = item_id(name);
        if (id) ids.emplace_back(id);
        else printf("[event] item not found: '%s'\n", name);
    }
    if (!d.gems && ids.empty()) return false;

    active_event a{ w.name, d.title, d.seconds ? std::time(nullptr) + d.seconds : 0, {} };
    int spawned = 0;

    auto drop = [&](short id, short amount, bool track)
    {
        int x, y;
        if (!random_air(w, x, y)) return;
        const int uid = add_object(ev, ::slot(id, amount), ::pos{ x * 32.0f + 8.0f, y * 32.0f + 8.0f }, w);
        if (track) a.uids.emplace_back(static_cast<u_int>(uid));
        ++spawned;
    };

    if (d.gems)
        for (int i = 0; i < 30; ++i) drop(112, static_cast<short>(RandomRange(1, 6)), false);

    if (d.mixed)
        for (int i = 0; i < d.count; ++i) drop(ids[RandomRange(0, static_cast<int>(ids.size()))], 1, true);
    else
        for (short id : ids)
            for (int i = 0; i < d.count; ++i) drop(id, 1, true);

    if (spawned == 0) return false;

    const std::string text = std::format("`2{}:`` `o{}``", d.title, d.text);
    peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &peer)
    {
        send_varlist(&peer, { "OnAddNotification", "interface/large/special_event.rttex", text, "audio/cumbia_horns.wav", 0u });
        on::ConsoleMessage(&peer, text);
    });

    if (d.seconds > 0 && !a.uids.empty()) active.emplace_back(std::move(a));
    return true;
}

/* called when someone picks up a dropped object */
void event_object_taken(ENetEvent &event, ::world &w, u_int uid, short id)
{
    for (auto it = active.begin(); it != active.end(); ++it)
    {
        if (it->world != w.name) continue;
        auto u = std::ranges::find(it->uids, uid);
        if (u == it->uids.end()) continue;
        it->uids.erase(u);

        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        world_msg(w.name, std::format("{} `2found a`` `w{}``!", pPeer->display_growid, id_to_item(id).raw_name));

        if (it->uids.empty())
        {
            notify(w.name, std::format("`2{}:`` `oEverything was found! The event is over.``", it->title));
            active.erase(it);
        }
        return;
    }
}

/* how often each random event is picked compared to the others (default 10) */
static int event_weight(const event_def &d)
{
    const std::string_view t{ d.title };
    if (t == "Jungle Blast!") return 1; // @note rare: about 1 in 121 events
    return 10;
}

static std::size_t pick_anytime(const std::vector<std::size_t> &anytime)
{
    int total = 0;
    for (std::size_t i : anytime) total += event_weight(defs[i]);
    int r = RandomRange(0, total);
    for (std::size_t i : anytime) { r -= event_weight(defs[i]); if (r < 0) return i; }
    return anytime.back();
}
/* runs once a second from tick_timers() */
void events_tick(std::time_t now)
{
    for (auto it = active.begin(); it != active.end();)
    {
        if (now < it->ends) { ++it; continue; }

        auto w = std::ranges::find(worlds, it->world, &::world::name);
        if (w != worlds.end())
        {
            ENetPeer *p = any_peer_in(it->world);
            for (u_int uid : it->uids)
            {
                auto o = std::ranges::find(w->objects, uid, &::object::uid);
                if (o == w->objects.end()) continue;
                if (p)
                {
                    ENetEvent ev{};
                    ev.peer = p;
                    item_change_object(ev, ::gamePacket{ .netid = static_cast<::peer*>(p->data)->netid, .uid = (int)0xffffffff, .id = (int)uid }); // @note same message as a pickup
                }
                w->objects.erase(o);
            }
            if (p) // @note the removal message looks like a pickup to that client (it adds the item), so send the real backpack again
            {
                ENetEvent ev{};
                ev.peer = p;
                send_inventory_state(ev);
                on::SetBux(ev);
            }
            notify(it->world, std::format("`2{}:`` `4Time's up!`` `oThe event is over.``", it->title));
        }
        it = active.erase(it);
    }

    static std::time_t next_roll = 0;
    if (now < next_roll) return;
    next_roll = now + 60;

    std::vector<std::size_t> anytime{};
    for (std::size_t i = 0; i < defs.size(); ++i) if (defs[i].anytime) anytime.emplace_back(i);

    for (::world &w : worlds)
    {
        if (event_running(w.name)) continue;
        const int here = static_cast<int>(peers(w.name, PEER_SAME_WORLD).size());
        if (here == 0) continue;
        if (RandomRange(0, 100) >= std::min(EVENT_CHANCE * here, 50)) continue;
        start_event(w, defs[pick_anytime(anytime)]);
    }
}

/* /event            -> list events
 * /event {number}   -> start that event in your world (developers) */
void event_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER)
    {
        send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
        return;
    }
    if (pPeer->netid == 0)
    {
        tell(event.peer, "`4You must be in a world.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg.empty())
    {
        std::string anytime{ "`oAnytime:``" }, holiday{ "`oHoliday:``" };
        for (std::size_t i = 0; i < defs.size(); ++i)
            (defs[i].anytime ? anytime : holiday) += std::format(" `w{}`` {}", i + 1, defs[i].title);
        on::ConsoleMessage(event.peer, anytime);
        on::ConsoleMessage(event.peer, holiday);
        on::ConsoleMessage(event.peer, "`oUse `w/event {number}`` to start one here.``");
        return;
    }

    const int n = std::atoi(arg.c_str()) - 1;
    if (n < 0 || n >= static_cast<int>(defs.size()))
    {
        tell(event.peer, "`4No event with that number.`` Type `w/event`` for the list.");
        return;
    }

    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;
    if (!start_event(*w, defs[n]))
        tell(event.peer, "`4Couldn't start that event - check the server window for missing items.``");
}


/* Mystery Lens: one random valuable wearable hidden in the world for 90 seconds */
bool event_hidden_object(::world &w)
{
    ENetPeer *p = any_peer_in(w.name);
    if (!p) return false;

    short prize = 0;
    for (int tries = 0; tries < 500 && !prize; ++tries)
    {
        const ::item &it = items[RandomRange(0, static_cast<int>(items.size()))];
        if (it.raw_name.empty() || it.type == type::SEED || it.cloth_type == clothing::NONE) continue;
        if (it.rarity < 30 || it.rarity > 200) continue;
        prize = static_cast<short>(it.id);
    }
    int x, y;
    if (!prize || !random_air(w, x, y)) return false;

    ENetEvent ev{};
    ev.peer = p;
    const int uid = add_object(ev, ::slot(prize, 1), ::pos{ x * 32.0f + 8.0f, y * 32.0f + 8.0f }, w);
    active.push_back({ w.name, "Hidden in Plain Sight!", std::time(nullptr) + 90, { static_cast<u_int>(uid) } });
    notify(w.name, "`2Hidden in Plain Sight!:`` `oSomething valuable is hidden somewhere in this world. You have `w90`` seconds to find it!``");
    return true;
}

/* Party-In-A-Box: starts the Anniversary Party event in this world */
bool event_party(::world &w)
{
    const auto d = std::ranges::find_if(defs, [](const event_def &e) { return std::string_view{ e.title } == "Anniversary Party!"; });
    return d != defs.end() && start_event(w, *d);
}