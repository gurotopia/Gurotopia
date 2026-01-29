#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "on/SetBux.hpp"
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
                .add_image_button("banner", "interface/large/gui_event_banner_aliens.rttex", "bannerlayout", "")
                .add_spacer("small")
                .add_textbox(std::format("`w{} {}{}: `5Space Cat!``|", month[time.tm_mon], time.tm_mday,
                    (time.tm_mday >= 11 && time.tm_mday <= 13) ? "th" :
                    (time.tm_mday % 10 == 1) ? "st" :
                    (time.tm_mday % 10 == 2) ? "nd" :
                    (time.tm_mday % 10 == 3) ? "rd" : "th"))
                .add_spacer("small")
                /* IOTM goes here. but it's P2W, so i will not add. */
                .add_textbox("From beyond the stars comes a new threat the `8Space Cat``! This curious critter may look cute, but her sharp claws will have you praying for mercy! Wrench yourself and choose `2Transform`` to unleash her power! Don't miss out and grab the `8Space Cat`` now! Available in the store until the 31st of January 2026!")
                .add_spacer("small")
                .add_textbox("To avoid refund fraud, the `8Space Cat`` will be untradeable from January 29th, 2026 and will become tradeable from June 1st, 2026.")
                .add_spacer("small")
                .add_textbox("Strap in and get ready because the `2Aliens`` have returned to Growtopia! Your favorite extraterrestrial guests are back, bringing a universe full of fun, surprises, and out-of-this-world rewards!")
                .add_spacer("small")
                .add_textbox("Equip your `8Alien Scanner`` to find `8Alien Landing Pods`` hidden in the trees you harvest! Each pod is packed with stellar rewards—happy harvesting, and may alien luck be with you!")
                .add_spacer("small")
                .add_textbox("Visit our Social Media pages for more Content!")
                .add_spacer("small")
                .add_image_button("gazette_DiscordServer", "interface/large/gazette/gazette_5columns_social_btn01.rttex", "7imageslayout20", "https://discord.com/invite/zzWHgzaF7J")
                .add_image_button("gazette_Instagram", "interface/large/gazette/gazette_5columns_social_btn02.rttex", "7imageslayout20", "https://www.instagram.com/growtopia")
                .add_image_button("gazette_TikTok", "interface/large/gazette/gazette_5columns_social_btn03.rttex", "7imageslayout20", "https://tiktok.com/@growtopia")
                .add_image_button("gazette_Twitch", "interface/large/gazette/gazette_5columns_social_btn04.rttex", "7imageslayout20", "https://www.twitch.tv/growtopiagameofficial")
                .add_image_button("gazette_Twitter", "interface/large/gazette/gazette_5columns_social_btn06.rttex", "7imageslayout20", "https://twitter.com/growtopiagame")
                .add_image_button("gazette_Youtube", "interface/large/gazette/gazette_5columns_btn04.rttex", "7imageslayout20", "https://www.youtube.com/growtopia_official")
                .add_image_button("gazette_Facebook", "interface/large/gazette/gazette_5columns_btn05.rttex", "7imageslayout20", "https://www.facebook.com/growtopia")
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