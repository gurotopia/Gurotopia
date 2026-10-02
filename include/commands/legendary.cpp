#include "pch.hpp"
#include <map>
#include <unordered_map>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
#include "onVariant/CountryState.hpp"
#include "tools/bubble.hpp"
#include "database/database.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "lockaccess.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "onVariant/EmoticonDataChanged.hpp"
#include "onVariant/Spawn.hpp"
#include "onVariant/BillboardChange.hpp"
#include "onVariant/SetClothing.hpp"
#include "commands/jammers.hpp"
#include "commands/weather.hpp"
#include "tools/time.hpp"
#include "commands/bot.hpp"
#include "commands/curse.hpp"
#include "commands/lockaccess.hpp"
#include "legendary.hpp"
#include "flag.hpp"

namespace
{
/* ================= quest data (growtopiawiki.com/w/Legendary_Quest) ================= */

enum class kind : u_char { DELIVER, GEMS, BREAK, BREAK_RARITY, PLANT_RARITY, FRUIT_RARITY, XP, TREES, SEEDS };

struct step
{
    kind k;
    long long amount;
    std::vector<std::string> names{}; // @note DELIVER: any one of these items
};

struct quest
{
    std::string title, reward, blurb;
    int reward_fallback_id; // @note used if the reward's name isn't found in items.dat
    std::vector<step> steps;
};

step D(long long n, std::initializer_list<const char*> names) { step s{ kind::DELIVER, n }; for (const char *x : names) s.names.emplace_back(x); return s; }
step G(long long n)  { return { kind::GEMS, n }; }
step B(long long n)  { return { kind::BREAK, n }; }
step BR(long long n) { return { kind::BREAK_RARITY, n }; }
step PR(long long n) { return { kind::PLANT_RARITY, n }; }
step FR(long long n) { return { kind::FRUIT_RARITY, n }; }
step XP(long long n) { return { kind::XP, n }; }
step T(long long n)  { return { kind::TREES, n }; }  // @note stands in for steps this server has no system for (PvP, surgery, villains, star voyages)
step S(long long n)  { return { kind::SEEDS, n }; }  // @note stands in for providers, Geiger Counter, fish training
step ORB()           { return D(1, { "Legendary Orb" }); }

const std::vector<quest> &quests()
{
    static const std::vector<quest> q{
        { "Quest For Honor", "", "the `9Legendary Title``: \"of Legend\" after your name", 0, {
            D(2000, {"Sand"}), T(100), B(5000), D(600, {"Display Box"}), PR(50000), D(50, {"Growtoken"}), D(3, {"Golden Diaper"}), XP(10000),
            D(1000, {"Tombstone"}), G(100000), BR(100000), T(500), S(1000), D(3, {"Golden Heart Crystal"}), FR(100000),
            D(1, {"Growie Award", "Neptune's Crown"}), D(3, {"Super Fireworks"}), D(10, {"Rainbow Wings"}), D(3, {"Birth Certificate"}), ORB() } },
        { "Quest For Fire", "Dragon of Legend", "the `9Dragon of Legend``", 1782, {
            D(2000, {"Lava"}), T(100), B(5000), D(600, {"Dragon Gate"}), PR(50000), D(50, {"Growtoken"}), D(10, {"Dragon Hand"}), XP(10000),
            D(1000, {"Dragon Tail"}), G(100000), BR(100000), T(500), S(1000), D(3, {"Fiesta Dragon"}), FR(100000),
            D(1, {"Ultra Trophy 3000"}), D(1, {"Neptune's Pendant"}), D(1000, {"Rocket Thruster"}), D(3, {"Devil Wings"}), ORB() } },
        { "Quest Of Steel", "Legendbot-009", "the `9Legendbot-009``", 1780, {
            D(2000, {"Chemical G"}), T(100), B(5000), D(600, {"Robot Wants Dubstep"}), PR(50000), D(50, {"Growtoken"}), D(3, {"Edison Zoomster"}), XP(10000),
            D(1000, {"High Tech Block"}), G(100000), BR(100000), T(500), S(1000), D(3, {"Bride Of Reanimator Remote"}), FR(100000),
            D(1, {"Mint Julep", "Neptune's Chariot"}), D(5, {"Kerjigger"}), D(5, {"Doohickey"}), D(2, {"Thingamabob"}), ORB() } },
        { "Quest Of The Heavens", "Legendary Wings", "the `9Legendary Wings``", 1784, {
            D(1000, {"Clouds"}), T(100), B(5000), D(600, {"Fairy Wings"}), PR(50000), D(50, {"Growtoken"}), D(3, {"Bubble Wings"}), XP(10000),
            D(800, {"Crimson Eagle Wings"}), G(100000), BR(100000), T(500), S(1000), D(20, {"Rainbow Wings"}), FR(100000),
            D(3, {"Golden Angel Wings"}), D(100, {"Ripper Wings"}), D(1, {"Phoenix Wings", "Neptune's Trident"}), D(50, {"Parrot Wings"}), ORB() } },
        { "Quest For The Blade", "Legendary Katana", "the `9Legendary Katana``", 2592, {
            D(1000, {"Iron Bars"}), T(100), B(5000), D(600, {"Golden Sword"}), PR(50000), D(50, {"Growtoken"}), D(3, {"Heavenly Scythe"}), XP(10000),
            D(800, {"Headsman's Axe"}), G(100000), BR(100000), T(500), S(1000), D(20, {"Flamesaber"}), FR(100000),
            D(1, {"Neptune's Gauntlet"}), D(10, {"Much-Too-Small Yellow Shirt"}), D(20, {"Flame Scythe"}), D(20, {"Crystal Glaive"}), ORB() } },
        { "Quest For Candour", "Whip of Truth", "the `9Whip of Truth``", 6026, {
            D(2000, {"Secret Of Growtopia"}), T(100), B(10000), D(1000, {"Mind-Ghost-In-A-Jar"}), PR(100000), D(50, {"Growtoken"}), D(10, {"Super Squirt Gun Jetpack"}), XP(20000),
            D(5, {"Soul Stone"}), G(140000), BR(200000), T(500), S(1000), D(3, {"Celestial Lance"}), FR(200000),
            D(5, {"Ring Of Shrinking"}), D(3, {"Golden Talaria"}), D(1, {"Summer Event Player Medal: Gold", "Winter Event Player Medal: Gold", "Spring Event Player Medal: Gold"}),
            D(3, {"Ancestral Totem of Wisdom"}), ORB() } },
        { "Quest For The Sky", "Legendary Dragon Knight's Wings", "the `9Legendary Dragon Knight's Wings``", 7734, {
            D(2000, {"Obsidian"}), T(100), B(10000), D(1000, {"Knight Helmet"}), PR(100000), D(50, {"Growtoken"}), D(10, {"Blanket Cape"}), XP(20000),
            D(800, {"Blazing Electro Wings"}), G(140000), BR(200000), T(500), S(1000), D(10, {"Autumn Wings"}), FR(200000),
            D(1, {"Golden Dragon Statue", "Neptune's Armor"}), D(5, {"Chaos Dragon"}), D(1, {"Draconic Wings"}), D(10, {"Dragon Knight's Chestplate"}), ORB() } },
        { "Quest Of The Owl", "Legendary Owl", "the `9Legendary Owl``", 11142, {
            D(2000, {"Clouds Wallpaper"}), T(100), B(10000), D(50, {"Owl Mask"}), PR(100000), D(50, {"Growtoken"}), D(50, {"Alaskan King Crab Crown"}), XP(20000),
            D(10, {"Sun Shooter Bow"}), G(100000), BR(200000), T(500), S(1000), D(1, {"Golden Silk Scarf", "Neptune's Weather Machine - Atlantis"}), FR(200000),
            D(2, {"Snow Leopard Tail"}), D(3, {"Ultraviolet Aura"}), D(1, {"Lil Growpeep's Baaaa Blaster"}), D(1, {"Draconic Soul Aura"}), ORB() } },
        { "Quest Of The Mech", "Legendary Destroyer", "the `9Legendary Destroyer``", 11140, {
            D(2000, {"Dwarven Background"}), T(100), B(10000), D(150, {"Lost Startopian Helmet"}), PR(100000), D(50, {"Growtoken"}), D(15, {"Monster Truck"}), XP(20000),
            D(10, {"Matrix Aura"}), G(100000), BR(200000), T(500), S(1000), D(3, {"Digger's Spade"}), FR(200000),
            D(10, {"Ambu-Lance"}), D(5, {"Mining Mech"}), D(1, {"Phoenix Armor"}), D(1, {"Volcanic Cape"}), ORB() } },
    };
    return q;
}

/* ================= item names -> ids (from the server's own items.dat) ================= */

std::string key_of(const std::string &s)
{
    std::string k{};
    for (char c : s) if (std::isalnum(static_cast<unsigned char>(c))) k += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return k;
}

int item_id(const std::string &name)
{
    static std::unordered_map<std::string, int> ids{};
    if (ids.empty())
        for (const ::item &it : items)
            if (it.id != 0 && !it.raw_name.empty()) ids.try_emplace(key_of(it.raw_name), static_cast<int>(it.id));
    auto it = ids.find(key_of(name));
    return it == ids.end() ? 0 : it->second;
}

int wizard_id() { static const int id = [] { const int i = item_id("Legendary Wizard"); return i ? i : 1790; }(); return id; }

int reward_id(const quest &q)
{
    if (q.reward.empty()) return 0;
    const int id = item_id(q.reward);
    return id ? id : q.reward_fallback_id;
}

/* ================= per-player state ================= */

enum title_bits : int { LEGEND_UNLOCKED = 1, LEGEND_ON = 2, DR_UNLOCKED = 4, DR_ON = 8 };

struct lstate
{
    int quest{ -1 }; // @note -1 = none
    int step{};      // @note 0-based
    long long progress{};
    int titles{};
    bool dirty{};
    bool loaded{};
};
std::map<int, lstate> states{};

void ensure_table()
{
    static bool done = false;
    if (done) return;
    done = true;
    mysql_query(db, "CREATE TABLE IF NOT EXISTS legendary (uid INT PRIMARY KEY, quest INT NOT NULL DEFAULT -1, step INT NOT NULL DEFAULT 0, "
                    "progress BIGINT NOT NULL DEFAULT 0, titles INT NOT NULL DEFAULT 0)");
}

lstate &state_of(int uid)
{
    lstate &s = states[uid];
    if (s.loaded) return s;
    s.loaded = true;
    ensure_table();
    if (mysql_query(db, std::format("SELECT quest, step, progress, titles FROM legendary WHERE uid = {}", uid).c_str()) == 0)
        if (MYSQL_RES *res = mysql_store_result(db))
        {
            if (MYSQL_ROW r = mysql_fetch_row(res))
            {
                s.quest = r[0] ? std::atoi(r[0]) : -1;
                s.step = r[1] ? std::atoi(r[1]) : 0;
                s.progress = r[2] ? std::atoll(r[2]) : 0;
                s.titles = r[3] ? std::atoi(r[3]) : 0;
            }
            mysql_free_result(res);
        }
    if (s.quest >= static_cast<int>(quests().size())) s.quest = -1;
    return s;
}

void save(int uid)
{
    lstate &s = state_of(uid);
    ensure_table();
    mysql_query(db, std::format("REPLACE INTO legendary (uid, quest, step, progress, titles) VALUES ({}, {}, {}, {}, {})", uid, s.quest, s.step, s.progress, s.titles).c_str());
    s.dirty = false;
}

std::string num(long long n)
{
    std::string s = std::to_string(n), out{};
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (i != 0 && (s.size() - i) % 3 == 0) out += ',';
        out += s[i];
    }
    return out;
}

std::string step_text(const step &st)
{
    switch (st.k)
    {
        case kind::DELIVER:
        {
            std::string names{};
            for (std::size_t i = 0; i < st.names.size(); ++i) names += (i == 0 ? "" : (i + 1 == st.names.size() ? " or " : ", ")) + ("`w" + st.names[i] + "``");
            return std::format("Deliver {} {}.", num(st.amount), names);
        }
        case kind::GEMS:         return std::format("Deliver {} Gems.", num(st.amount));
        case kind::BREAK:        return std::format("Break {} blocks.", num(st.amount));
        case kind::BREAK_RARITY: return std::format("Break {} rarity worth of blocks.", num(st.amount));
        case kind::PLANT_RARITY: return std::format("Plant seeds that add up to {} rarity.", num(st.amount));
        case kind::FRUIT_RARITY: return std::format("Collect {} rarity worth of fruit from trees.", num(st.amount));
        case kind::XP:           return std::format("Earn {} XP.", num(st.amount));
        case kind::TREES:        return std::format("Harvest {} trees.", num(st.amount));
        case kind::SEEDS:        return std::format("Plant {} seeds.", num(st.amount));
    }
    return "";
}

bool item_missing(const step &st)
{
    if (st.k != kind::DELIVER) return false;
    for (const std::string &n : st.names) if (item_id(n)) return false;
    return true;
}

int owned(const ::peer &p, int id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}

void broadcast(const std::string &text)
{
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *o = static_cast<::peer*>(p.data);
        if (o && !o->growid.empty()) on::ConsoleMessage(&p, text);
    });
}

/* where each player last talked to a wizard (it vanishes when a quest is turned in there) */
struct spot { std::string world; int x, y; };
std::map<int, spot> talked_at{};

/* completes the current step; at the end of the quest hands out the reward */
void complete_step(ENetEvent &event, bool at_wizard)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    lstate &s = state_of(p->user_id);
    const quest &q = quests()[s.quest];

    s.progress = 0;
    ++s.step;
    send_action(*event.peer, "play_sfx", "file|audio/levelup2.wav\ndelayMS|0");

    if (s.step < static_cast<int>(q.steps.size()))
    {
        on::ConsoleMessage(event.peer, std::format("`9Legendary Quest:`` step `w{}/20`` complete! Next: {}", s.step, step_text(q.steps[s.step])));
        send_varlist(event.peer, { "OnTextOverlay", std::format("`9Step {} of {} complete!``", s.step, q.title) });
        save(p->user_id);
        return;
    }

    // the whole quest is done
    const int finished = s.quest;
    s.quest = -1;
    s.step = 0;
    if (q.reward.empty())
    {
        s.titles |= LEGEND_UNLOCKED | LEGEND_ON;
        if (p->netid != 0)
            if (auto w = std::ranges::find(worlds, p->recent_worlds.back(), &::world::name); w != worlds.end()) refresh_display_names(*w);
        on::CountryState(event);
    }
    else if (const int id = reward_id(q); id > 0)
        modify_item_inventory(event, ::slot(static_cast<short>(id), 1));
    save(p->user_id);

    send_varlist(event.peer, { "OnAddNotification", "interface/large/special_event.rttex", std::format("`9You completed the {}!``", q.title), "audio/cumbia_horns.wav", 0u });
    broadcast(std::format("`9** {} has completed the {} and earned {}! **``", p->growid, q.title, quests()[finished].blurb));

    // the wizard it was turned in at vanishes (so don't give others access to yours)
    if (at_wizard)
        if (auto t = talked_at.find(p->user_id); t != talked_at.end() && p->netid != 0 && t->second.world == p->recent_worlds.back())
            if (auto w = std::ranges::find(worlds, t->second.world, &::world::name); w != worlds.end())
            {
                const int x = t->second.x, y = t->second.y;
                if (y >= 0 && cord(x, y) < static_cast<int>(w->blocks.size()) && w->blocks[cord(x, y)].fg == wizard_id())
                {
                    ::block &b = w->blocks[cord(x, y)];
                    b.fg = 0;
                    ::gamePacket gp{};
                    gp.punch = ::pos{ x, y };
                    send_tile_update(event, gp, b, *w);
                    on::ConsoleMessage(event.peer, "`oThe Legendary Wizard vanishes in a puff of smoke.``");
                }
            }
}

void counter(ENetEvent &event, kind k, long long n)
{
    if (n <= 0 || event.peer == nullptr || event.peer->data == nullptr) return;
    ::peer *p = static_cast<::peer*>(event.peer->data);
    if (p->user_id == 0) return;
    lstate &s = state_of(p->user_id);
    if (s.quest < 0) return;
    const step &st = quests()[s.quest].steps[s.step];
    if (st.k != k) return;
    s.progress += n;
    s.dirty = true;
    if (s.progress >= st.amount) complete_step(event, false);
}

/* ================= wizard dialogs ================= */

void send_dialog(ENetEvent &event, const std::string &body)
{
    send_varlist(event.peer, { "OnDialogRequest", "set_default_color|`o\n" + body });
}

void greeting(ENetEvent &event)
{
    std::string d = std::format("add_label_with_icon|big|`9The Legendary Wizard``|left|{}|\n", wizard_id());
    d += "add_textbox|`oGreetings, traveler! I am the Legendary Wizard. Should you wish to embark on a Legendary Quest, simply choose one below.``|left|\nadd_spacer|small|\n";
    for (std::size_t i = 0; i < quests().size(); ++i) d += std::format("add_button|q_{}|`9{}``|noflags|0|0|\n", i, quests()[i].title);
    d += "end_dialog|legendary_wizard|No Thanks||\n";
    send_dialog(event, d);
}

void describe(ENetEvent &event, int qi)
{
    const quest &q = quests()[qi];
    std::string d = std::format("add_label_with_icon|big|`9{}``|left|{}|\n", q.title, wizard_id());
    d += std::format("add_textbox|`oThis quest will challenge every fiber of your Growtopian being. It will cost you thousands of gems, weeks or months of time, and possibly your friends and family.<CR>"
                     "Every quest has 20 steps to complete, and each step alone is probably more than most Growtopians could manage.<CR>"
                     "But the rewards are also vast. If you complete this quest, you will earn {}!<CR>"
                     "These quest rewards are `5Untradeable``, and you will truly be a Legendary Growtopian if you complete a quest.<CR>"
                     "You may turn in your quests at any Legendary Wizard you have access to (we're in a union), but I will vanish permanently if somebody turns in their final quest step to me, so don't let other people access me!``|left|\n", q.blurb);
    d += "add_spacer|small|\n";
    d += std::format("add_button|begin_{}|`9Begin the {}!``|noflags|0|0|\n", qi, q.title);
    d += "add_button|back|Back|noflags|0|0|\nend_dialog|legendary_wizard|No Thanks||\n";
    send_dialog(event, d);
}

void step_view(ENetEvent &event)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    lstate &s = state_of(p->user_id);
    const quest &q = quests()[s.quest];
    const step &st = q.steps[s.step];

    std::string d = std::format("add_label_with_icon|big|`9{}``|left|{}|\n", q.title, wizard_id());
    d += std::format("add_textbox|`o(Step {}/20)``|left|\nadd_spacer|small|\nadd_textbox|`o{}``|left|\n", s.step + 1, step_text(st));
    if (s.step == 1 || s.step == 11 || s.step == 12)
        d += "add_smalltext|`o(This server has no PvP, surgery, villains, providers or fish training, so this step stands in for the original one.)``|left|\n";
    d += "add_spacer|small|\n";
    d += std::format("add_textbox|`oCurrent progress: `w{}/{}``|left|\n", num(s.progress), num(st.amount));

    if (item_missing(st))
        d += "add_textbox|`4That item doesn't exist in this server's items.dat.``|left|\nadd_button|skip|`oSkip this step``|noflags|0|0|\n";
    else if (st.k == kind::DELIVER)
    {
        long long have = 0;
        std::string name{};
        for (const std::string &n : st.names) if (const int id = item_id(n); id && owned(*p, id) > 0) { have = owned(*p, id); name = n; break; }
        const long long give = std::min(have, st.amount - s.progress);
        if (give > 0) d += std::format("add_button|deliver|`oDeliver {} {}``|noflags|0|0|\n", num(give), name);
        else d += "add_smalltext|`oYou don't have any of that with you.``|left|\n";
    }
    else if (st.k == kind::GEMS)
    {
        const long long give = std::min<long long>(p->gems, st.amount - s.progress);
        if (give > 0) d += std::format("add_button|deliver|`oDeliver {} Gems``|noflags|0|0|\n", num(give));
        else d += "add_smalltext|`oYou don't have any gems with you.``|left|\n";
    }
    else d += "add_smalltext|`oThis step counts up by itself while you play - come back when it's done, or keep an eye on the chat.``|left|\n";

    d += "add_spacer|small|\nadd_button|give_up|`oGive up this quest``|noflags|0|0|\nend_dialog|legendary_wizard|Goodbye!||\n";
    send_dialog(event, d);
}

bool can_use_wizard(const ::peer &p, const ::world &w)
{
    if (p.role >= MODERATOR || w.owner == 0 || w.owner == p.user_id) return true;
    return std::ranges::find(w.access, p.user_id) != w.access.end();
}
}

/* ================= titles ================= */

void legend_load(const ::peer &p) { if (p.user_id) (void)state_of(p.user_id); }

void legend_save(const ::peer &p)
{
    if (!p.user_id) return;
    if (auto it = states.find(p.user_id); it != states.end())
    {
        if (it->second.dirty) save(p.user_id);
        states.erase(it);
    }
}

void legend_tick(std::time_t now)
{
    static std::time_t next = 0;
    if (now < next) return;
    next = now + 60;
    for (auto &[uid, s] : states) if (s.dirty) save(uid);
}

namespace
{
/* ---- the titles ----
 * flag: what the game itself draws (tested on the 5.58 client). "" = no game look, done with name colours */
enum more_title_bits : int { G4G_UNLOCKED = 16, G4G_ON = 32, MENTOR_UNLOCKED = 64, MENTOR_ON = 128,
                             PARTY_UNLOCKED = 256, PARTY_ON = 512, MAXLVL_UNLOCKED = 1024, MAXLVL_HIDDEN = 2048 };

struct title_def
{
    const char *key;   // @note /title {key}
    const char *label; // @note shown in the Title window and in messages
    const char *desc;
    int unlocked;
    int on;            // @note the bit that turns it on, or hides it when 'inverted'
    const char *flag;  // @note OnCountryState flag
    bool inverted;     // @note on unless the bit is set (level 125 is on by default)
};

enum title_index { T_LEGEND, T_DR, T_MAXLVL, T_G4G, T_MENTOR, T_PARTY };
constexpr title_def title_defs[] = {
    { "legend",   "`9of Legend``",   "Legendary look and \"of Legend\" after your name", LEGEND_UNLOCKED, LEGEND_ON,     "legend",   false },
    { "dr",       "`4Dr.``",         "red name with \"Dr.\" in front",                    DR_UNLOCKED,     DR_ON,         "doctor",   false },
    { "maxlevel", "`1Level 125``",   "pulsing blue name",                                MAXLVL_UNLOCKED, MAXLVL_HIDDEN, "maxLevel", true  },
    { "g4g",      "`2Grow4Good``",   "pastel name and the Grow4Good badge",              G4G_UNLOCKED,    G4G_ON,        "donor",    false },
    { "mentor",   "`6Mentor``",      "gold name and the mentor medal",                   MENTOR_UNLOCKED, MENTOR_ON,     "master",   false },
    { "party",    "`5Party Animal``", "purple, yellow and green letters",                PARTY_UNLOCKED,  PARTY_ON,      "",         false },
};
constexpr const char *title_list = "legend, dr, maxlevel, g4g, mentor, party";

const title_def *find_title(std::string k)
{
    for (char &c : k) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (k == "legendary") k = "legend";
    else if (k == "doctor") k = "dr";
    else if (k == "grow4good") k = "g4g";
    else if (k == "level125" || k == "125" || k == "max") k = "maxlevel";
    else if (k == "partyanimal") k = "party";
    for (const title_def &d : title_defs) if (k == d.key) return &d;
    return nullptr;
}

bool has_title(const ::peer &p, int t, const title_def &d)
{
    if (t & d.unlocked) return true;
    return d.unlocked == MAXLVL_UNLOCKED && p.level.front() >= 125; // @note level 125 earns it by itself
}

bool title_on(const ::peer &p, int t, const title_def &d)
{
    if (!has_title(p, t, d)) return false;
    return d.inverted ? !(t & d.on) : (t & d.on) != 0;
}

void set_title_on(int &t, const title_def &d, bool enable)
{
    if (d.inverted == enable) t &= ~d.on;
    else t |= d.on;
}

bool any_title(const ::peer &p, int t)
{
    for (const title_def &d : title_defs) if (has_title(p, t, d)) return true;
    return false;
}

bool any_title_on(const ::peer &p, int t)
{
    for (const title_def &d : title_defs) if (title_on(p, t, d)) return true;
    return false;
}

/* Party Animal: every letter purple, yellow, green in turn */
std::string party_colours(const std::string &s)
{
    static constexpr char cols[] = { '5', '9', '2' };
    std::string out{};
    std::size_t n = 0;
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '`') { ++i; continue; } // @note drop the old colour codes
        if (s[i] != ' ') { out += '`'; out += cols[n++ % 3]; }
        out += s[i];
    }
    return out + "``";
}
}

std::string title_name(const ::peer &p, const std::string &base)
{
    if (p.user_id == 0) return base;
    const int t = state_of(p.user_id).titles;
    std::string name = base;
    if (title_on(p, t, title_defs[T_DR])) // @note red comes from the doctor flag; "Dr." goes right before the name, after colour code and @
    {
        std::size_t at = (name.size() >= 2 && name[0] == '`') ? 2 : 0;
        if (name.size() > at && name[at] == '@') ++at;
        name.insert(at, "Dr.");
    }
    if (title_on(p, t, title_defs[T_LEGEND]))
    {
        while (name.ends_with("``")) name.resize(name.size() - 2);
        name += " of Legend``";
    }
    if (title_on(p, t, title_defs[T_PARTY])) name = party_colours(name);
    return name;
}

std::string country_state_for(const ::peer &p)
{
    std::string s = flag_of(p); // @note /flag
    const int t = p.user_id ? state_of(p.user_id).titles : 0;
    for (const title_def &d : title_defs)
        if (*d.flag && title_on(p, t, d)) s += std::string("|") + d.flag;
    return s;
}

void titles_open(ENetEvent &event)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    const int t = state_of(p->user_id).titles;
    std::string d = "add_label_with_icon|big|`wTitles``|left|1794|\nadd_spacer|small|\n";
    for (const title_def &x : title_defs)
        if (has_title(*p, t, x)) d += std::format("add_checkbox|t_{}|{} - {}|{}\n", x.key, x.label, x.desc, title_on(*p, t, x) ? 1 : 0);
    if (!any_title(*p, t))
        d += "add_textbox|`oYou haven't earned any titles yet. Complete the `9Quest For Honor`` at a Legendary Wizard to earn the Legendary title.``|left|\n";
    d += "add_spacer|small|\nend_dialog|title_edit|Cancel|OK|\n";
    send_dialog(event, d);
}

/* shows a title change to everyone in the world: the new name first, then the flags.
 * The game redraws the title look from that pair (same as entering a world); no respawn */
static void apply_titles(ENetPeer &who)
{
    ::peer *p = static_cast<::peer*>(who.data);
    ENetEvent ev{};
    ev.peer = &who;
    if (p->netid == 0) { on::CountryState(ev); return; }

    auto w = std::ranges::find(worlds, p->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) { on::CountryState(ev); return; }
    p->display_growid = display_name_for(*p, *w); // @note /nick included

    const bool hidden = (p->state & S_INVISIBLE) == S_INVISIBLE;
    peers(w->name, PEER_SAME_WORLD, [&](ENetPeer &o)
    {
        const bool self = &o == &who;
        if (!self && hidden) return;
        send_varlist(&o, { "OnNameChanged", p->display_growid }, p->netid);
        if (self) on::CountryState(ev);
        else on::CountryStateOf(o, *p);
    });
}

void titles_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    lstate &s = state_of(p->user_id);
    const int before = s.titles;
    for (const title_def &x : title_defs)
    {
        if (!has_title(*p, s.titles, x)) continue;
        const std::string box = std::string("t_") + x.key;
        const std::string v = hPipe[box.c_str()];
        if (v == "1") set_title_on(s.titles, x, true);
        else if (v == "0") set_title_on(s.titles, x, false);
    }
    if (s.titles == before) return;
    save(p->user_id);
    apply_titles(*event.peer);
}

std::string title_quick_button(const ::peer &p)
{
    const int t = state_of(p.user_id).titles;
    if (!any_title(p, t)) return "";
    return any_title_on(p, t) ? "add_button|title_toggle|`wTitle: `2ON`` - tap to turn off``|noflags|0|0|\n"
                              : "add_button|title_toggle|`wTitle: `4OFF`` - tap to turn on``|noflags|0|0|\n";
}

/* all your titles on or all off */
static void set_all_titles(ENetEvent &event, bool enable)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    lstate &s = state_of(p->user_id);
    if (!any_title(*p, s.titles)) { on::ConsoleMessage(event.peer, "`oYou don't have any titles yet.``"); return; }
    const int before = s.titles;
    for (const title_def &x : title_defs) if (has_title(*p, s.titles, x)) set_title_on(s.titles, x, enable);
    if (s.titles == before) return;
    save(p->user_id);
    apply_titles(*event.peer);
    on::ConsoleMessage(event.peer, enable ? "`2Your titles are on.``" : "`oYour titles are off.``");
}

void title_toggle(ENetEvent &event)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    set_all_titles(event, !any_title_on(*p, state_of(p->user_id).titles));
}

/* /title on|off                   - all your own titles
 * /title {title}                  - switch one of your own titles
 * /title {player} {title} [off]   - developers give or take a title */
void title_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *me = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args{};
    for (const std::string &a : readch(std::string{ text }, ' ')) if (!a.empty()) args.push_back(a);
    if (args.size() == 2 && (args[1] == "on" || args[1] == "off")) { set_all_titles(event, args[1] == "on"); return; }
    if (args.size() == 2)
    {
        if (const title_def *x = find_title(args[1]))
        {
            lstate &st = state_of(me->user_id);
            if (!has_title(*me, st.titles, *x)) { on::ConsoleMessage(event.peer, std::format("`4You don't have the {} title.``", x->label)); return; }
            const bool now_on = !title_on(*me, st.titles, *x);
            set_title_on(st.titles, *x, now_on);
            save(me->user_id);
            apply_titles(*event.peer);
            on::ConsoleMessage(event.peer, std::format("`o{} is now {}.", x->label, now_on ? "`2on``" : "`4off``"));
            return;
        }
    }
    if (me->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, std::format("`4Usage: /title {{title}}`` to switch one title, `4/title on`` or `4/title off`` for all. Titles: {}", title_list));
        return;
    }
    const title_def *x = args.size() >= 3 ? find_title(args[2]) : nullptr;
    if (!x)
    {
        on::ConsoleMessage(event.peer, std::format("`4Usage: /title {{player}} {{title}} [off]`` - titles: {}", title_list));
        return;
    }
    const bool off = args.size() >= 4 && args[3] == "off";

    std::string want = args[1];
    for (char &c : want) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    ENetPeer *target = nullptr;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;
        std::string n = t->growid;
        for (char &c : n) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (n == want) target = &p;
    });
    if (!target) { on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not online.``", args[1])); return; }

    ::peer *t = static_cast<::peer*>(target->data);
    lstate &s = state_of(t->user_id);
    if (off)
    {
        s.titles &= ~x->unlocked;
        set_title_on(s.titles, *x, false); // @note a level 125 player keeps the title, but it stays hidden
    }
    else
    {
        s.titles |= x->unlocked;
        set_title_on(s.titles, *x, true);
    }
    save(t->user_id);
    apply_titles(*target);
    on::ConsoleMessage(event.peer, std::format("`2{} the {} title {} `w{}``.``", off ? "Took" : "Gave", x->label, off ? "from" : "to", t->growid));
    if (target != event.peer)
        on::ConsoleMessage(target, off ? std::format("`oYour {} title was removed.``", x->label)
                                       : std::format("`2You earned the {} title!`` Wrench yourself and tap Title, or type `w/title {}``.", x->label, x->key));
}

/* ================= wizard ================= */

bool wizard_wrench(ENetEvent &event, ::world &world, int x, int y)
{
    if (y < 0 || cord(x, y) >= static_cast<int>(world.blocks.size()) || world.blocks[cord(x, y)].fg != wizard_id()) return false;
    ::peer *p = static_cast<::peer*>(event.peer->data);
    if (!can_use_wizard(*p, world))
    {
        send_varlist(event.peer, { "OnTalkBubble", p->netid, "`wThe Legendary Wizard only talks to people with access to this world.``", 0u });
        return true;
    }
    talked_at[p->user_id] = { world.name, x, y };
    if (state_of(p->user_id).quest < 0) greeting(event);
    else step_view(event);
    return true;
}

void wizard_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    const std::string btn = hPipe["buttonClicked"];
    if (btn.empty()) return;

    // still standing near a wizard they may use?
    auto t = talked_at.find(p->user_id);
    if (t == talked_at.end() || p->netid == 0 || t->second.world != p->recent_worlds.back()) return;
    auto w = std::ranges::find(worlds, t->second.world, &::world::name);
    if (w == worlds.end() || cord(t->second.x, t->second.y) >= static_cast<int>(w->blocks.size()) || w->blocks[cord(t->second.x, t->second.y)].fg != wizard_id()) return;

    lstate &s = state_of(p->user_id);

    if (s.quest < 0)
    {
        if (btn == "back") { greeting(event); return; }
        if (btn.starts_with("q_")) { const int qi = std::atoi(btn.c_str() + 2); if (qi >= 0 && qi < static_cast<int>(quests().size())) describe(event, qi); return; }
        if (btn.starts_with("begin_"))
        {
            const int qi = std::atoi(btn.c_str() + 6);
            if (qi < 0 || qi >= static_cast<int>(quests().size())) return;
            if (p->level.front() < 40 && p->role < MODERATOR)
            {
                send_dialog(event, std::format("add_label_with_icon|big|`9The Legendary Wizard``|left|{}|\nadd_textbox|`oYou are not ready yet, young one. Come back when you are at least `wlevel 40``.``|left|\nend_dialog|legendary_wizard||OK|\n", wizard_id()));
                return;
            }
            s.quest = qi;
            s.step = 0;
            s.progress = 0;
            save(p->user_id);
            on::ConsoleMessage(event.peer, std::format("`9You have begun the {}!`` Good luck, you'll need it.", quests()[qi].title));
            step_view(event);
        }
        return;
    }

    const quest &q = quests()[s.quest];
    const step &st = q.steps[s.step];

    if (btn == "give_up")
    {
        send_dialog(event, std::format("add_label_with_icon|big|`9{}``|left|{}|\nadd_textbox|`4Are you sure?`` All progress on this quest will be lost, and anything you delivered is gone.|left|\n"
                                       "add_button|give_up_yes|`4Yes, give up``|noflags|0|0|\nadd_button|back|No, keep going|noflags|0|0|\nend_dialog|legendary_wizard|||\n", q.title, wizard_id()));
        return;
    }
    if (btn == "give_up_yes")
    {
        on::ConsoleMessage(event.peer, std::format("`oYou gave up the {}.``", q.title));
        s.quest = -1; s.step = 0; s.progress = 0;
        save(p->user_id);
        return;
    }
    if (btn == "back") { step_view(event); return; }
    if (btn == "skip" && item_missing(st)) { complete_step(event, true); if (s.quest >= 0) step_view(event); return; }
    if (btn != "deliver") return;

    if (st.k == kind::GEMS)
    {
        const long long give = std::min<long long>(p->gems, st.amount - s.progress);
        if (give <= 0) return;
        p->gems -= static_cast<int>(give);
        on::SetBux(event);
        s.progress += give;
    }
    else if (st.k == kind::DELIVER)
    {
        long long give = 0;
        for (const std::string &n : st.names)
            if (const int id = item_id(n); id && owned(*p, id) > 0)
            {
                give = std::min<long long>(owned(*p, id), st.amount - s.progress);
                modify_item_inventory(event, ::slot(static_cast<short>(id), static_cast<short>(-give)));
                break;
            }
        if (give <= 0) return;
        s.progress += give;
    }
    else return;

    s.dirty = true;
    if (s.progress >= st.amount) complete_step(event, true);
    else save(p->user_id);
    if (s.quest >= 0) step_view(event);
}

/* ================= counters ================= */

void legend_on_break(ENetEvent &event, int rarity) { counter(event, kind::BREAK, 1); counter(event, kind::BREAK_RARITY, rarity); }
void legend_on_plant(ENetEvent &event, int rarity) { counter(event, kind::SEEDS, 1); counter(event, kind::PLANT_RARITY, rarity); }
void legend_on_harvest(ENetEvent &event, int fruit_rarity) { counter(event, kind::TREES, 1); counter(event, kind::FRUIT_RARITY, fruit_rarity); }
void legend_on_xp(ENetEvent &event, int xp) { counter(event, kind::XP, xp); }

bool staff_only_item(int id)
{
    if (id == 274 || id == 276 || id == 278 || id == 732) return true; // @note Freeze, Fire, Curse and Ban Wand
    for (const quest &q : quests()) if (!q.reward.empty() && reward_id(q) == id) return true;
    return false;
}
