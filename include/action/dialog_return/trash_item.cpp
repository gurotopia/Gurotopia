#include "pch.hpp"
#include <unordered_map>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
#include "database/shouhin.hpp"
#include "tools/random.hpp"

#include "trash_item.hpp"

/* gems for recycling: items sold in the store give 10% of the store price (World Lock 2000 -> 200),
 * everything else gives random gems by rarity, like breaking a block */
static int recycle_gems(short id, int count)
{
    static std::unordered_map<short, double> store_value{};
    if (store_value.empty())
    {
        for (const auto &[tab, s] : shouhin_tachi)
            if (s.items.size() == 1 && s.cost > 0 && s.items.front().second > 0)
                store_value.try_emplace(s.items.front().first, s.cost * 0.10 / s.items.front().second);
        store_value.try_emplace(0, 0.0); // @note so an empty store isn't rebuilt every time
    }
    if (const auto v = store_value.find(id); v != store_value.end()) return static_cast<int>(std::lround(v->second * count));

    const ::item &item = id_to_item(static_cast<u_short>(id));
    const int per_item =
        (item.rarity >= 87) ? 22 :
        (item.rarity >= 68) ? 18 :
        (item.rarity >= 53) ? 14 :
        (item.rarity >= 41) ? 11 :
        (item.rarity >= 36) ? 10 :
        (item.rarity >= 32) ? 9 :
        (item.rarity >= 24) ? 5 : 1;
    int gems = 0;
    for (int i = 0; i < count; ++i) gems += RandomRange(0, per_item);
    return gems;
}

void trash_item(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short itemID = static_cast<short>(atoi(hPipe["itemID"].c_str()));
    int count = atoi(hPipe["count"].c_str());
    if (itemID <= 0 || itemID >= static_cast<int>(items.size()) || count <= 0) return;

    const ::item &item = id_to_item(itemID);
    if (item.type == type::FIST || item.type == type::WRENCH) return;

    int have = 0;
    for (const ::slot &slot : pPeer->slots) if (slot.id == itemID) have = slot.count;
    if (have <= 0) return;
    count = std::min(count, have);

    const int gems = recycle_gems(itemID, count);
    modify_item_inventory(event, ::slot(itemID, static_cast<short>(-count)));

    if (gems > 0)
    {
        pPeer->gems += gems;
        on::SetBux(event);
    }
    on::ConsoleMessage(event.peer, std::format("{} `w{}`` recycled, `w{}`` gems earned.", count, item.raw_name, gems));
}
