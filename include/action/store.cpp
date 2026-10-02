#include "pch.hpp"
#include "database/shouhin.hpp"
#include "store.hpp"

/* the Home tab: popular items that can be bought right there */
static constexpr const char *featured[]{
    "world_lock", "upgrade_backpack", "small_seed_pack", "signal_jammer", "punch_jammer", "zombie_jammer",
    "door_mover", "change_of_address", "deluxe_grow_spray", "antigravity_generator", "treasure_blast", "cave_blast"
};

void action::store(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(header, '|');
    if (!header.empty() && (pipes.size() < 4 || pipes[3] != "gem")) return; // @note location|gem

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const int No = (pPeer->slot_size - 16) / 10 + 1;

    std::string s =
        "set_description_text|Welcome to the `2Growtopia Store``! Here are some popular items - browse the tabs for everything else.\n"
        "enable_tabs|1\n"
        "add_tab_button|main_menu|Home|interface/large/btn_shop.rttex||1|0|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_tab_button|locks_menu|Locks And Stuff|interface/large/btn_shop.rttex||0|1|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_tab_button|itempack_menu|Item Packs|interface/large/btn_shop.rttex||0|3|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_tab_button|bigitems_menu|Awesome Items|interface/large/btn_shop.rttex||0|4|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_tab_button|weather_menu|Weather Machines|interface/large/btn_shop.rttex|Tired of the same sunny sky?  We offer alternatives within...|0|5|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_tab_button|token_menu|Growtoken Items|interface/large/btn_shop.rttex||0|2|0|0||||-1|-1|||0|0|CustomParams:|\n"
        "add_banner|interface/large/gui_shop_featured_header.rttex|0|1|\n";

    for (const char *btn : featured)
        for (const auto &[tab, sh] : shouhin_tachi)
        {
            if (sh.btn != btn) continue;
            int cost = sh.cost;
            if (sh.btn == "upgrade_backpack")
            {
                if (No > 38) break; // @note maxed out
                cost = 100 * No * No - 200 * No + 200;
            }
            s += std::format("add_button|{}|{}|{}|{}|{}|{}|{}|0|||-1|-1||-1|-1||1||||||0|0|CustomParams:|\n",
                sh.btn, sh.name, sh.rttx, sh.description, sh.tex1, sh.tex2, cost);
            break;
        }

    send_varlist(event.peer, { "OnStoreRequest", s });
}
