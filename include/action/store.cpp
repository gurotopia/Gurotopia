#include "pch.hpp"
#include "store.hpp"

#include "tools/string.hpp"

std::string payment_site{ "https://discord.gg/zzWHgzaF7J" }; // @note replace with your payment site (paypal, ect)

void action::store(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(std::move(header), '|');

    if (header.empty() || pipes[3] == "gem") // @note location|gem
    {
        packet::create(*event.peer, false, 0, 
        {
            "OnStoreRequest",
            std::format(
                "set_description_text|Welcome to the `2Growtopia Store``! Select the item you'd like more info on.`o `wThanks for being a supporter of Growtopia!\n"
            
                /* tabs */
                "enable_tabs|1\n"
                "add_tab_button|main_menu|Home|interface/large/btn_shop.rttex||1|0|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|locks_menu|Locks And Stuff|interface/large/btn_shop.rttex||0|1|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|itempack_menu|Item Packs|interface/large/btn_shop.rttex||0|3|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|bigitems_menu|Awesome Items|interface/large/btn_shop.rttex||0|4|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|weather_menu|Weather Machines|interface/large/btn_shop.rttex|Tired of the same sunny sky?  We offer alternatives within...|0|5|0|0||||-1|-1|||0|0|CustomParams:|\n"
                "add_tab_button|token_menu|Growtoken Items|interface/large/btn_shop.rttex||0|2|0|0||||-1|-1|||0|0|CustomParams:|\n"

                "add_banner|interface/large/gui_shop_featured_header.rttex|0|1|\n"
                "add_button|pet_battle_pack|`oPet Battle Pack``|interface/large/store_buttons/store_buttons42.rttex|`2You Get:`` 1 Pet Battle Supply Crate, 1 Battle Leash and 1 Battle Pet Cage.<CR><CR>`5Description:`` When a Growtopian plucks up the courage, they can get their own battle pet. This pack contains all the ingredients to create your very own starter Battle Pet and get you ready for your very first Pet Battle. Splicing new battle abilities will cost 10 World Locks per ability, World Locks not included!<CR><CR>`4This pack can be only be purchased once.``|1|6|40000|0|||-1|-1||-1|-1||1||||||0|0|CustomParams:|\n"
                "add_button|rt_grope_supply_bundle05|`oScrolls Value Pack``|interface/large/store_buttons/store_buttons42.rttex|{0}|2|1|0||||-1|-1||-1|-1|OPENDIALOG&showscrollspurchaseui|1||||||0|0|CustomParams:|\n"
                "add_button|rt_grope_supply_bundle04|`oDungeon Pass``|interface/large/store_buttons/store_buttons42.rttex|{0}|1|5|0||||-1|-1||-1|-1|OPENDIALOG&showdungeonpasspurchaseui|1||||||0|0|CustomParams:|\n"
                "add_button|rt_grope_battlepass_bundle01|Royal Grow Pass|interface/large/store_buttons/store_buttons37.rttex|{0}|4|2|0||||-1|-1||-1|-1|`2You Get:`` 1 Royal Grow Pass Token.<CR><CR>`5Description:`` Play to earn points and level up your Grow Pass to earn rewards. Consume to earn exclusive `5Royal`` rewards as you level up your Grow Pass as well as unlocking all daily bonuses and exclusive `5Royal Perks`` for the entire month. Upon consuming you will instantly receive `2300 Free`` points towards your pass progress. Note: The token is `#UNTRADEABLE``.|1||||||0|0|CustomParams:|\n"
                "add_banner|interface/large/gui_shop_featured_header.rttex|0|2|\n"
                "add_button|gems_glory|Road To Glory|interface/large/store_buttons/store_buttons30.rttex|{0}|0|0|0||||-1|-1|/interface/large/gui_shop_buybanner.rttex|1|0|`2You Get:`` Road To Glory and 120k Gems Instantly.<CR>`5Description:`` Earn Gem rewards when you level up. Every 10 levels you will get additional Gem rewards up to Level 50! Claim all rewards instantly if you are over level 50!! 1.92M Gems in total!!|1||||||0|0|CustomParams:|\n"
                "add_button|gems_bundle06|Gem Abundance|interface/large/store_buttons/store_buttons37.rttex|{0}|2|1|0||||-1|-1||-1|-1|`2You Get:`` 19,680,000 Gems, 3,680 World Locks, 10 Growtokens and 5 Megaphones.<CR><CR>`5Description:`` Get an abundance of gems to add to your wealth. Please note World Locks will be credited as 36 Diamond Locks and 80 World Locks. Please make sure you have room in your inventory to receive these items.|1||||||0|0|CustomParams:|\n"
                "add_button|gems_bundle05|Gem Bounty|interface/large/store_buttons/store_buttons34.rttex|{0}|0|6|0||||-1|-1||-1|-1|`2You Get:`` 10,080,000 Gems, 2,100 World Locks, 6 Growtokens and 3 Megaphones.<CR><CR>`5Description:`` Get a plethora of gems to add to your wealth. Please note World Locks will be credited as 21 Diamond Locks. Please make sure you have room in your inventory to receive these items.|1||||||0|0|CustomParams:|\n"
                "add_button|gems_rain|It's Rainin' Gems|interface/large/store_buttons/store_buttons.rttex|{0}|1|5|0||||-1|-1||-1|-1|`2You Get:`` 2,730,000 Gems, 630 World Locks, 2 Growtokens and 1 Megaphone.<CR><CR>`5Description:`` All the gems you could ever want and more plus 630 World Locks, 2 Growtokens and a Megaphone to tell the whole world about it. Please note World Locks will be credited as 6 Diamond Locks and 30 World Locks. Please make sure you have room in your inventory to receive these items.|1||||||0|0|CustomParams:|\n"
                "add_button|gems_fountain|Gem Fountain|interface/large/store_buttons/store_buttons2.rttex|{0}|0|2|0||||-1|-1||-1|-1|`2You Get:`` 922,500 Gems, 260 World Locks and 1 Growtoken.<CR><CR>`5Description:`` Get a pile of Gems to shop to your hearts desire, 260 World Locks and 1 Growtoken. Please note World Locks will be credited as 2 Diamond Locks and 60 World Locks. Please make sure you have room in your inventory to receive these items.|1||||||0|0|CustomParams:|\n"
                "add_button|gems_chest|Chest o' Gems|interface/large/store_buttons/store_buttons.rttex|{0}|0|5|0||||-1|-1||-1|-1|`2You Get:`` 280,000 Gems and 84 World Locks.<CR><CR>`5Description:`` Get a chest containing gems and 84 World Locks. Please make sure you have room in your inventory to receive these items.|1||||||0|0|CustomParams:|\n"
                "add_button|grow_shop|`oGrowtopia Shop``|interface/large/store_buttons/store_buttons36.rttex|https://growtopiagame.com/shop|1|1|0|0|Open Shop||-1|-1|interface/large/gui_shop_buybanner.rttex|0|1|If you can purchase Gem Packs through your telecom provider, then open the Growtopia Shop website here.<CR><CR>Please note, this service is only available in the following countries: `9Brazil``, `9Cambodia``, `9Estonia``, `9Indonesia``, `9Latvia``, `9Lithuania``, `9Malaysia``, `9Philippines``, `9Qatar``, `9Romania``, `9Saudi Arabia``, `9Singapore``, `9Thailand``, `9Turkey``, and `9United Arab Emirates``|1||||||0|0|CustomParams:|\n"
                "add_banner|interface/large/gui_shop_featured_header.rttex|0|3|\n"
                "add_image_button|image_button_battlepass_banner|interface/large/gui_shop_grow_pass.rttex|bannerlayout|OPENDIALOG|battlepasspopup||\n"
                "add_button|365d|`o1-Year Subscription Token``|interface/large/store_buttons/store_buttons28.rttex|{0}|0|2|0||||-1|-1||-1|-1|`2You Get:`` 1x 1-Year Subscription Token , and 33 Growtokens. <CR><CR>`5Description:`` One full year of special treatment AND 33 Growtokens upfront! You'll get 70 season tokens (as long as there's a seasonal clash running), 9600 gems, and 1 World Lock every day and a chance of doubling any XP earned, growtime reduction on all seeds planted and Exclusive Skins!|1||||||0|0|CustomParams:|\n"
                "add_button|rt_grope_battlepass_bundle01|Royal Grow Pass|interface/large/store_buttons/store_buttons37.rttex|{0}|4|2|0||||-1|-1||-1|-1|`2You Get:`` 1 Royal Grow Pass Token.<CR><CR>`5Description:`` Play to earn points and level up your Grow Pass to earn rewards. Consume to earn exclusive `5Royal`` rewards as you level up your Grow Pass as well as unlocking all daily bonuses and exclusive `5Royal Perks`` for the entire month. Upon consuming you will instantly receive `2300 Free`` points towards your pass progress. Note: The token is `#UNTRADEABLE``.|1||||||0|0|CustomParams:|\n"
                "add_button|redeem_code|Redeem Code|interface/large/store_buttons/store_buttons40.rttex|OPENDIALOG&showredeemcodewindow|1|5|0|0|||-1|-1||-1|-1||1||||||0|0|CustomParams:|\n"
                ,payment_site
            ).c_str()
        });
    }
}