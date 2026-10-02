#include "pch.hpp"
#include <fstream>
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "tools/random.hpp"
#include "database/items.hpp"
#include "buffs.hpp"
#include "scrolls.hpp"

static constexpr short SECRET       = 1050; // @note Secret Of Growtopia
static constexpr short MAP_FRAGMENT = 7590;

static int count_of(const ::peer &p, short id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}

/* half written tips, half real facts from items.dat */
static std::string random_secret()
{
    static const char *lines[] = {
        "Lock your world before you build anything you'd miss.",
        "Nobody who asks to 'hold' your items plans to give them back.",
        "Rarer trees give more XP when you harvest them.",
        "Two seeds spliced together can grow something neither could alone.",
        "The Chemical Combiner rewards the curious.",
        "Punching blocks is honest work. Punching friends is less so.",
        "Lava doesn't care how rare your clothes are.",
        "Some treasure chests are buried deeper than others.",
        "Type /? to see what you can do.",
        "Growganoth remembers every offering.",
        "If you can read this, you just threw a secret to the winds.",
        "The deepest caves hide golden hoards.",
        "Beach treasure lies under the sand, not in the sea.",
        "Snowy peaks guard the oldest secrets.",
        "Wizards never explain their wands.",
        "A hundred scraps of paper can point the way to treasure.",
        "Fertilizer is patience in a bottle.",
        "Shh. This one stays between us.",
    };
    static const std::vector<std::string> from_file = []
    {
        std::vector<std::string> v{};
        std::ifstream in("secrets.txt");
        for (std::string line; std::getline(in, line); )
        {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
            if (!line.empty()) v.push_back(line);
        }
        (void)0; // @note debug line removed
        return v;
    }();
    if (RandomRange(0, 2) == 0)
        return from_file.empty() ? std::string{ lines[RandomRange(0, static_cast<int>(std::size(lines)))] }
                                 : from_file[RandomRange(0, static_cast<int>(from_file.size()))];

    for (int tries = 0; tries < 50; ++tries)
    {
        const ::item &it = items[RandomRange(0, static_cast<int>(items.size()))];
        if (it.raw_name.empty() || it.type == type::SEED) continue;
        switch (RandomRange(0, 3))
        {
            case 0: if (it.rarity > 0 && it.rarity < 999) return std::format("{} has a rarity of {}.", it.raw_name, it.rarity); break;
            case 1: if (it.splice[0] && it.splice[1]) return std::format("{} can be spliced from {} and {}.", it.raw_name, id_to_item(it.splice[0]).raw_name, id_to_item(it.splice[1]).raw_name); break;
            case 2: if (!it.splice[0]) return std::format("{} can't be created by splicing seeds!", it.raw_name); break;
        }
    }
    return lines[0];
}

bool scroll_use(ENetEvent &event, const ::item &item)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (item.id == SECRET)
    {
        modify_item_inventory(event, ::slot(SECRET, -1));
        pPeer->add_xp(event, consumable_xp(*pPeer));
        on::ConsoleMessage(event.peer, std::format("`1The Secret Of Growtopia reveals:`` `w{}``", random_secret()));
        return true;
    }
    if (item.id == MAP_FRAGMENT)
    {
        const int have = count_of(*pPeer, MAP_FRAGMENT);
        if (have < 100)
        {
            tell(event.peer, std::format("`4You need `w100`` Map Fragments to piece together a map (you have {}).``", have));
            return true;
        }
        const int roll = RandomRange(0, 100);
        const short map = roll < 60 ? 7602 /*Treasure Map*/ : roll < 69 ? 7604 /*Map of Tyr*/ : roll < 78 ? 7606 /*Map of Odin*/
                        : roll < 87 ? 7608 /*Map of Thor*/ : roll < 96 ? 7610 /*Map of Frigg*/ : 7612 /*Legendary Map - Untouched*/;
        modify_item_inventory(event, ::slot(MAP_FRAGMENT, -100));
        modify_item_inventory(event, ::slot(map, 1));
        tell(event.peer, std::format("`2You pieced together a `w{}``!``", id_to_item(static_cast<u_short>(map)).raw_name));
        return true;
    }
    return false;
}
