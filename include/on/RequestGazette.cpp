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
        .add_textbox("Love is popping, hearts are dropping, and we are officially in the `@Valentine's`` spirit! `@Valentine's Week`` has arrived, bringing sweet surprises, shiny rewards ready to steal your heart!")
        .add_spacer("small")
        .add_textbox("Taking center stage is the fabulous `9Golden Heart Crystal``, a true treasure of love that can be used to create dazzling gold items! Finding it won't be easy, but then again, true love never is! Keep your eyes peeled while breaking a `pHeartstone``, `8Golden Booty Chest``, `8Super Golden Booty Chest``, or taking a chance at the `#Well of Love``, you never know when luck strikes!")
        .add_spacer("small")
        .add_textbox("If your heart beats for competition, the `cEssence of Love Daily Challenge`` is calling your name! Show your dedication, place in the top 5, and you'll be rewarded with a `9Golden Heart Crystal`` as proof that love really does pay off!")
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

    packet::create(*event.peer, false, 0, {
        "OnDialogRequest",
        dialog
    });
}
