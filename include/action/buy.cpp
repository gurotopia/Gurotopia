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

    short tab{};
    if (pipes[3] == "main") action::store(event, ""); // tab = 0
    else if (pipes[3] == "locks") tab = 1;
    else if (pipes[3] == "itempack") tab = 2;
    if (tab != 0) 
    {
        std::string StoreRequest{};

        StoreRequest.append(
            (tab == 1) ? "set_description_text|`2Locks And Stuff!``  Select the item you'd like more info on, or BACK to go back.\n" :
            (tab == 2) ? "set_description_text|`2Item Packs!``  Select the item you'd like more info on, or BACK to go back.\n" : ""
        );
        StoreRequest.append("enable_tabs|1\nadd_tab_button|main_menu|Home|interface/large/btn_shop.rttex||0|0|0|0||||-1|-1|||0|0|CustomParams:|\n");
        StoreRequest.append(
            std::format(
                "add_tab_button|locks_menu|Locks And Stuff|interface/large/btn_shop.rttex||{}|1|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|itempack_menu|Item Packs|interface/large/btn_shop.rttex||{}|3|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|bigitems_menu|Awesome Items|interface/large/btn_shop.rttex||0|4|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|weather_menu|Weather Machines|interface/large/btn_shop.rttex|Tired of the same sunny sky?  We offer alternatives within...|0|5|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|token_menu|Growtoken Items|interface/large/btn_shop.rttex||0|2|0|0||||-1|-1|||0|0|CustomParams:|\n",
                (tab == 1) ? "1" : "0", (tab == 2) ? "1" : "0"
        ));
        for (auto &&[_tab, shouhin] : shouhin_tachi)
        {
            if (_tab == tab)
            {
                if (shouhin.btn == "upgrade_backpack") shouhin.cost = backpack_cost; // @todo race-condition for multiple people accessing store
                StoreRequest.append(std::format(
                    "add_button|{}|{}|{}|{}|{}|{}|{}|0|||-1|-1||-1|-1||1||||||0|0|CustomParams:|\n",
                    shouhin.btn, shouhin.name, shouhin.rttx, shouhin.description, shouhin.tex1, shouhin.tex2, shouhin.cost
                ));
            }
        }
        packet::create(*event.peer, false, 0, { "OnStoreRequest", StoreRequest.c_str() });
        return;
    }
    for (auto &&[_tab, shouhin] : shouhin_tachi) // @todo only iterate if peer is actually buying a item.
    {
        if (pipes[3] == shouhin.btn)
        {
            if (shouhin.btn == "upgrade_backpack") shouhin.cost = backpack_cost;
            if (peer->gems < shouhin.cost) 
            {
                packet::create(*event.peer, false, 0, 
                {
                    "OnStorePurchaseResult",
                    std::format("You can't afford `0{}``!  You're `${}`` Gems short.", shouhin.name, shouhin.cost - peer->gems).c_str()
                });
                return;
            }

            std::string received{};
            for (std::pair<short, short> &im : shouhin.im)
            {
                if (im.first == 9412) peer->slot_size += 10; // @note 9412 is the id for increase backpack sprite, but peer wont actually be given that item.
                else peer->emplace(slot(im.first, im.second));
                received.append(std::format("{}, ", items[im.first].raw_name)); // @todo add green text to rare items, or something cool.
            }
            inventory_visuals(event);

            packet::create(*event.peer, false, 0, 
            {
                "OnStorePurchaseResult",
                std::format(
                    "You've purchased `0{}`` for `${}`` Gems.\nYou have `${}`` Gems left.\n\n`5Received: ```0{}``",
                    shouhin.name, shouhin.cost, peer->gems -= shouhin.cost, received
                ).c_str()
            });
            on::SetBux(event);

            break;
        }
    }
}