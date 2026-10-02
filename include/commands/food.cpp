#include "pch.hpp"
#include "tools/bubble.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "tools/random.hpp"
#include "food.hpp"
#include "buffs.hpp"

enum class taste : u_char { FOOD, DRINK, FIZZY, SPICY, GROSS, COLD, GUM, HEART };

/* plain food and drinks, by item id (from this server's items.dat) */
static const std::unordered_map<u_short, taste> menu{
    /* food */
    {406, taste::FOOD}, {676, taste::FOOD}, {950, taste::FOOD}, {958, taste::FOOD}, {964, taste::FOOD}, {966, taste::FOOD},
    {968, taste::FOOD}, {1208, taste::FOOD}, {1374, taste::FOOD}, {1580, taste::FOOD}, {2002, taste::FOOD}, {2734, taste::FOOD},
    {3420, taste::FOOD}, {3428, taste::FOOD}, {3474, taste::FOOD}, {3546, taste::FOOD}, {3600, taste::FOOD}, {3816, taste::FOOD},
    {3836, taste::FOOD}, {4256, taste::FOOD}, {4410, taste::FOOD}, {4422, taste::FOOD}, {4602, taste::FOOD}, {4666, taste::FOOD},
    {4668, taste::FOOD}, {4672, taste::FOOD}, {4764, taste::FOOD}, {4982, taste::FOOD}, {5256, taste::FOOD}, {7346, taste::FOOD},
    {7638, taste::FOOD}, {10270, taste::FOOD}, {10628, taste::FOOD}, {10988, taste::FOOD}, {14246, taste::FOOD}, {4592, taste::FOOD}, {5114, taste::DRINK}, {10490, taste::DRINK}, {7464, taste::FOOD}, {7058, taste::FOOD}, {6912, taste::FOOD}, {4232, taste::FOOD}, {6046, taste::FOOD}, {6048, taste::FOOD}, {6050, taste::FOOD}, {6910, taste::FOOD}, {7466, taste::DRINK},
    /* drinks */
    {868, taste::DRINK}, {1602, taste::DRINK}, {1634, taste::DRINK}, {3622, taste::DRINK}, {4984, taste::DRINK},
    {7052, taste::DRINK}, {11068, taste::DRINK}, {3240, taste::DRINK},
    /* fizzy */
    {3740, taste::FIZZY}, {4230, taste::FIZZY},
    /* spicy */
    {712, taste::SPICY}, {960, taste::SPICY}, {4258, taste::SPICY}, {8378, taste::SPICY},
    /* gross */
    {8380, taste::GROSS}, {8382, taste::GROSS}, {8384, taste::GROSS}, {8386, taste::GROSS}, {8400, taste::GROSS},
    {8402, taste::GROSS}, {8404, taste::GROSS}, {8406, taste::GROSS}, {8408, taste::GROSS}, {8410, taste::GROSS},
    {8412, taste::GROSS}, {8414, taste::GROSS},
    /* cold */
    {3228, taste::COLD}, {6314, taste::COLD}, {6316, taste::COLD},
    /* other */
    {1772, taste::GUM}, {386, taste::HEART},
};

static const char *pick(std::initializer_list<const char*> lines)
{
    const int i = RandomRange(0, static_cast<int>(lines.size()));
    return *(lines.begin() + i);
}

static std::string bubble_for(taste t)
{
    switch (t)
    {
        case taste::FOOD:  return "YUM!";
        case taste::DRINK: return "YUM!";
        case taste::FIZZY: return pick({ "*BURP*", "*burp* ...excuse me." });
        case taste::SPICY: return pick({ "`4HOT HOT HOT!``", "`4*breathes fire*``", "`4WATER! I NEED WATER!``" });
        case taste::GROSS: return pick({ "*gags*", "Blegh!", "Why did I eat that..." });
        case taste::COLD:  return pick({ "`1BRAIN FREEZE!``", "`1*shivers*``" });
        case taste::GUM:   return pick({ "*blows a big bubble*", "*pop*" });
        case taste::HEART: return std::format("`4{}``", pick({ "BE MINE", "LUV U", "XOXO", "HUG ME", "CUTIE PIE", "TRUE LOVE", "SO FINE", "SWEET TALK" }));
    }
    return "*nom*";
}

/* eat or drink a plain food item. @return true if handled */
bool food_use(ENetEvent &event, const ::item &item)
{
    const auto it = menu.find(item.id);
    if (it == menu.end()) return false;

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    modify_item_inventory(event, ::slot(static_cast<short>(item.id), -1));
    pPeer->add_xp(event, consumable_xp(*pPeer));

    const std::string text = bubble_for(it->second);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        send_varlist(&p, { "OnTalkBubble", speaker(*pPeer).netid, text, 0u }); // @note used on another player: they say it
    });
    return true;
}
