#include "pch.hpp"
#include "store.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "database/shouhin.hpp"
#include "tools/ransuu.hpp"
#include "buy.hpp"


void action::buy(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(std::move(header), '|');

    auto &peer = _peer[event.peer];

    short No = (peer->slot_size - 16) / 10 + 1; // @note number of upgrades | credits: https://growtopia.fandom.com/wiki/Backpack_Upgrade
    short backpack_cost = (100 * No * No - 200 * No + 200);

    auto growtoken = std::ranges::find_if(peer->slots, [](::slot &slot) { return slot.id == 1486; });

    short tab{};
    if (pipes[3] == "main") action::store(event, ""); // tab = 0
    else if (pipes[3] == "locks") tab = 1;
    else if (pipes[3] == "itempack") tab = 2;
    else if (pipes[3] == "bigitems") tab = 3;
    else if (pipes[3] == "weather") tab = 4;
    else if (pipes[3] == "token") tab = 5;
    if (tab != 0) 
    {
        std::string StoreRequest{};
        StoreRequest.append(
            (tab == 1) ? "set_description_text|`2Locks And Stuff!``  Select the item you'd like more info on, or BACK to go back.\n" :
            (tab == 2) ? "set_description_text|`2Item Packs!``  Select the item you'd like more info on, or BACK to go back.\n" :
            (tab == 3) ? "set_description_text|`2Awesome Items!``  Select the item you'd like more info on, or BACK to go back.\n" :
            (tab == 4) ? "set_description_text|`2Weather Machines!``  Select the item you'd like more info on, or BACK to go back.\n" :
            (tab == 5) ? 
                std::format(
                    "set_description_text|`2Spend your Growtokens!`` (You have `5{}``) You earn Growtokens from Crazy Jim and Sales-Man. Select the item you'd like more info on, or BACK to go back.\n",
                    (growtoken != peer->slots.end()) ? growtoken->count : 0) : ""
        );
        StoreRequest.append("enable_tabs|1\nadd_tab_button|main_menu|Home|interface/large/btn_shop.rttex||0|0|0|0||||-1|-1|||0|0|CustomParams:|\n");
        StoreRequest.append(
            std::format(
                "add_tab_button|locks_menu|Locks And Stuff|interface/large/btn_shop.rttex||{}|1|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|itempack_menu|Item Packs|interface/large/btn_shop.rttex||{}|3|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|bigitems_menu|Awesome Items|interface/large/btn_shop.rttex||{}|4|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|weather_menu|Weather Machines|interface/large/btn_shop.rttex|Tired of the same sunny sky?  We offer alternatives within...|{}|5|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|token_menu|Growtoken Items|interface/large/btn_shop.rttex||{}|2|0|0||||-1|-1|||0|0|CustomParams:|\n",
                (tab == 1) ? "1" : "0", (tab == 2) ? "1" : "0", (tab == 3) ? "1" : "0", (tab == 4) ? "1" : "0", (tab == 5) ? "1" : "0"
        ));
        for (auto &&[_tab, shouhin] : shouhin_tachi)
        {
            if (_tab == tab)
            {
                if (shouhin.btn == "upgrade_backpack") 
                {
                    if (No > 38) continue; // @note hide upgrade_backpack in store if maxed out
                    shouhin.cost = backpack_cost;
                }
                StoreRequest.append(std::format(
                    "add_button|{}|{}|{}|{}|{}|{}|{}|0|||-1|-1||-1|-1||1||||||0|0|CustomParams:|\n",
                    shouhin.btn, shouhin.name, shouhin.rttx, shouhin.description, shouhin.tex1, shouhin.tex2, shouhin.cost
                ));
            }
        }
        packet::create(*event.peer, false, 0, { "OnStoreRequest", StoreRequest.c_str() });
        return;
    }
    else for (auto &&[_tab, shouhin] : shouhin_tachi)
    {
        if (pipes[3] == shouhin.btn)
        {
            int growtoken_cost = std::abs(shouhin.cost);
            if (shouhin.btn == "upgrade_backpack") shouhin.cost = backpack_cost;
            if ((_tab == 5)
                ? (growtoken == peer->slots.end() || growtoken->count < growtoken_cost)
                : peer->gems < shouhin.cost)
            {
                packet::create(*event.peer, false, 0, 
                {
                    "OnStorePurchaseResult",
                    (_tab < 5) ? 
                        std::format("You can't afford `0{}``!  You're `${}`` Gems short.", 
                            shouhin.name, shouhin.cost - peer->gems).c_str() :
                        std::format("You can't afford `0{}``!  You're `${}`` `2Growtokens`` short.", 
                            shouhin.name, (growtoken == peer->slots.end()) ? growtoken_cost : growtoken_cost - growtoken->count).c_str()
                });
                return;
            }
            srand(std::time(0));
            std::vector<short> ids{};
            if (shouhin.btn == "basic_splice") // @note source: https://growtopia.fandom.com/wiki/Basic_Splicing_Kit
            {
                shouhin.im.emplace_back(11, 10);
                ids = {3567, 2793, 57, 13, 17, 21, 101, 381, 1139}; // @note instead of iterating seeds with rarity 2 each time
                for (std::size_t i = 0; i < 10; ++i)
                    shouhin.im.emplace_back(ids[rand() % ids.size()], 1);
            }
            else if (shouhin.btn == "rare_seed") // @note source: https://growtopia.fandom.com/wiki/Rare_Seed_Pack
            {
                for (auto &&[id, item] : items)
                    if (item.type == std::byte{ SEED } && item.rarity >= 13 && item.rarity <= 60)
                        ids.emplace_back(id);
                for (std::size_t i = 0; i < 5; ++i)
                    shouhin.im.emplace_back(ids[rand() % ids.size()], 1);
            }
            else if (shouhin.btn == "clothes_pack") // @note source: https://growtopia.fandom.com/wiki/Clothes_Pack
            {
                for (auto &&[id, item] : items)
                    if (item.type == std::byte{ CLOTHING } && item.rarity <= 10)
                        ids.emplace_back(id);
                for (std::size_t i = 0; i < 3; ++i)
                    shouhin.im.emplace_back(ids[rand() % ids.size()], 1);
            }
            else if (shouhin.btn == "rare_clothes_pack") // @note source: https://growtopia.fandom.com/wiki/Rare_Clothes_Pack
            {
                for (auto &&[id, item] : items)
                    if (item.type == std::byte{ CLOTHING } && item.rarity >= 11 && item.rarity <= 60)
                        ids.emplace_back(id);
                for (std::size_t i = 0; i < 3; ++i)
                    shouhin.im.emplace_back(ids[rand() % ids.size()], 1);
            }

            std::string received{};
            for (std::pair<short, short> &im : shouhin.im)
            {
                if (im.first == 9412) peer->slot_size += 10; // @note 9412 is the id for increase backpack sprite, but peer wont actually be given that item.
                else peer->emplace(slot(im.first, im.second));
                received.append(std::format("{}, ", items[im.first].raw_name)); // @todo add green text to rare items, or something cool.
            }

            packet::create(*event.peer, false, 0, 
            {
                "OnStorePurchaseResult",
                (_tab < 5) ? 
                    std::format(
                        "You've purchased `0{}`` for `${}`` Gems.\nYou have `${}`` Gems left.\n\n`5Received: ```0{}``",
                        shouhin.name, shouhin.cost, peer->gems -= shouhin.cost, received).c_str() :
                    std::format(
                        "You've purchased `0{}`` for `${}`` `2Growtokens``.\nYou have `${}`` `2Growtokens`` left.\n\n`5Received: ```0{}``",
                        shouhin.name, growtoken_cost, growtoken->count -= growtoken_cost, received).c_str()
            });
            inventory_visuals(event);
            on::SetBux(event); // @todo wasteful if peer is buying with growtokens

            break;
        }
    }
}