#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "on/SetBux.hpp"
#include "automate/holiday.hpp"

#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, false, 0, { "OnFtueButtonDataSet", 0, 0, 0, "||0|||-1", "", "1|1" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "ScrollsPurchaseButton", 1, "{\"active\":true,\"buttonAction\":\"showdungeonsui\",\"buttonState\":0,\"buttonTemplate\":\"DungeonEventButton\",\"counter\":20,\"counterMax\":20,\"itemIdIcon\":0,\"name\":\"ScrollsPurchaseButton\",\"notification\":0,\"order\":30,\"rcssClass\":\"scrollbank\",\"text\":\"20/20\"}" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "PiggyBankButton", 1, "{\"active\":true,\"buttonAction\":\"openPiggyBank\",\"buttonState\":0,\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":0,\"name\":\"PiggyBankButton\",\"notification\":0,\"order\":20,\"rcssClass\":\"piggybank\",\"text\":\"0/1.5M\"}" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "MailboxButton", 1, "{\"active\":true,\"buttonAction\":\"show_mailbox_ui\",\"buttonState\":0,\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":0,\"name\":\"MailboxButton\",\"notification\":0,\"order\":30,\"rcssClass\":\"mailbox\",\"text\":\"\"}" });

    if (peer->slots.empty()) // @note if peer has no items: assume they are a new player.
    {
        peer->emplace({18, 1}); // @note Fist
        peer->emplace({32, 1}); // @note Wrench
        peer->emplace({9640, 1}); // @note My First World Lock
    }

    peer->prefix = (peer->role == MODERATOR) ? "#@" : (peer->role == DEVELOPER) ? "8@" : peer->prefix;
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Welcome back, `{}{}````. No friends are online.", 
            peer->prefix, peer->ltoken[0]).c_str()
    }); 
    if (holiday == H_VALENTINES) packet::create(*event.peer, false, 0, {"OnConsoleMessage", "``4Happy Valentine's Week!``"});

    packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`5Personal Settings active:`` `#Can customize profile``"});
    
    send_inventory_state(event);
    on::SetBux(event);
    packet::create(*event.peer, false, 0, { "OnSetPearl", 0, 1 });
    packet::create(*event.peer, false, 0, { "OnSetVouchers", 0 });
    
    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, peer->ltoken[0].c_str(), ""}); 

    {
        std::time_t now = std::time(nullptr);
        std::tm time = *std::localtime(&now);
        std::vector<std::string> month = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

        packet::create(*event.peer, false, 0, {
            "OnTodaysDate",
            time.tm_mon + 1,
            time.tm_mday,
            0u, // @todo
            0u // @todo
        }); 

        on::RequestWorldSelectMenu(event);

        /* @todo move to a function and call in /news & here */
        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            ::create_dialog()
                .set_default_color("`o")
                .add_label_with_icon("big", "`wThe Gurotopia Gazette``", 5016)
                .add_spacer("small")
                .add_image_button("banner", "interface/large/gui_valentine_banner.rttex", "bannerlayout", "")
                .add_spacer("small")
                .add_textbox(std::format("`w{} {}{}: {}|", month[time.tm_mon], time.tm_mday,
                    (time.tm_mday >= 11 && time.tm_mday <= 13) ? "th" :
                    (time.tm_mday % 10 == 1) ? "st" :
                    (time.tm_mday % 10 == 2) ? "nd" :
                    (time.tm_mday % 10 == 3) ? "rd" : "th", (holiday == H_VALENTINES) ? "`5Valentine's Week!``" : ""))
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
                .end_dialog("gazette", "", "OK").c_str()
        });
    } // @note delete now, time

    send_data(*event.peer, compress_state(::state{
        .type = 0x16 // @noote PACKET_PING_REQUEST
    }));
}