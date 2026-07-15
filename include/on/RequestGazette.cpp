#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "automate/holiday.hpp"

#include "RequestGazette.hpp"

void on::RequestGazette(ENetEvent& event)
{
    std::tm time = localtime();
    std::vector<std::string> month = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

    std::string dialog = ::create_dialog()
        .set_default_color("`o")
        .add_label_with_icon("big", "`wThe Gurotopia Gazette``", 5016)
        .add_spacer("small")
        .add_image_button("banner", holiday_banner(), "bannerlayout", "")
        .add_spacer("small")
        .add_textbox(std::format("`w{} {}{}: {}|", month[time.tm_mon], time.tm_mday,
            (time.tm_mday >= 11 && time.tm_mday <= 13) ? "th" :
            (time.tm_mday % 10 == 1) ? "st" :
            (time.tm_mday % 10 == 2) ? "nd" :
            (time.tm_mday % 10 == 3) ? "rd" : "th", holiday_greeting().first))
        .add_spacer("small")
        /* IOTM goes here. but it's P2W, so i will not add. */
        .add_textbox("The sun is shining, the beaches are calling, and `2SummerFest`` is making waves once again! Dive into a season packed with sizzling adventures, sunny surprises, and fire-y rewards! As we head into Day 2 and beyond, it's time to unlock some of the biggest highlights of the event!")
        .add_spacer("small")
        .add_textbox("The Phoenix has officially risen! For a limited time, a select group of classic `4Phoenix Items`` will make their grand comeback! Whether you missed them the first time or are looking to complete your fiery collection, now's your chance to grab them while they're still around! Check the store for more info on how to get your hands on these rare treasures!")
        .add_spacer("small")
        .add_textbox("We're also adding `8Summer Artifact Chests`` with selected `2Gem Packs``! That's right, keep an eye out for bonus rewards when purchasing selected `2Gem Packs`` throughout the rest of the `2Summerfest``! You never know what summer treat might come your way!")
        .add_spacer("small")
        .add_textbox("Visit our Social Media pages for more Content!")
        .add_spacer("small")
        .add_image_button("gazette_DiscordServer", "interface/large/gazette/gazette_5columns_social_btn01.rttex", "7imageslayout", "https://discord.com/invite/zzWHgzaF7J")
        .add_layout_spacer("7imageslayout")
        .add_layout_spacer("7imageslayout")
        .add_layout_spacer("7imageslayout")
        .add_layout_spacer("7imageslayout")
        .add_layout_spacer("7imageslayout")
        .add_layout_spacer("7imageslayout")
        .add_spacer("small")
        .add_image_button("gazette_PrivacyPolicy", "interface/large/gazette/gazette_3columns_policy_btn02.rttex", "3imageslayout", "https://www.ubisoft.com/en-us/privacy-policy")
        .add_image_button("gazette_GrowtopianCode", "interface/large/gazette/gazette_3columns_policy_btn01.rttex", "3imageslayout", "https://support.ubi.com/en-us/growtopia-faqs/the-growtopian-code/")
        .add_image_button("gazette_TermsofUse", "interface/large/gazette/gazette_3columns_policy_btn03.rttex", "3imageslayout", "https://legal.ubi.com/termsofuse/")
        .add_quick_exit().add_spacer("small")
        .end_dialog("gazette", "", "OK");

    send_varlist(event.peer, { "OnDialogRequest", dialog });
}
