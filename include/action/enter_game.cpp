#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "on/RequestGazette.hpp"
#include "tools/string.hpp"
#include "on/SetBux.hpp"
#include "automate/holiday.hpp"

#include "enter_game.hpp"


void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, false, 0, { "OnFtueButtonDataSet", 0, 0, 0, "||0|||-1", "", "1|1" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "ScrollsPurchaseButton", 1, "{\"active\":true,\"buttonAction\":\"showdungeonsui\",\"buttonState\":0,\"buttonTemplate\":\"DungeonEventButton\",\"counter\":20,\"counterMax\":20,\"itemIdIcon\":0,\"name\":\"ScrollsPurchaseButton\",\"notification\":0,\"order\":30,\"rcssClass\":\"scrollbank\",\"text\":\"20/20\"}" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "PiggyBankButton", 1, "{\"active\":true,\"buttonAction\":\"openPiggyBank\",\"buttonState\":0,\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":0,\"name\":\"PiggyBankButton\",\"notification\":0,\"order\":20,\"rcssClass\":\"piggybank\",\"text\":\"0/1.5M\"}" });
    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "MailboxButton", 1, "{\"active\":true,\"buttonAction\":\"show_mailbox_ui\",\"buttonState\":0,\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":0,\"name\":\"MailboxButton\",\"notification\":0,\"order\":30,\"rcssClass\":\"mailbox\",\"text\":\"\"}" });

    if (pPeer->slots.empty())
    {
        pPeer->emplace({18, 1});
        pPeer->emplace({32, 1});
        pPeer->emplace({9640, 1});
    }

    pPeer->prefix = (pPeer->role == MODERATOR) ? "#@" : (pPeer->role == DEVELOPER) ? "8@" : pPeer->prefix;
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Welcome back, `{}{}````. No friends are online.", pPeer->prefix, pPeer->ltoken[0]).c_str()
    }); 
    packet::create(*event.peer, false, 0, {"OnConsoleMessage", holiday_greeting().second});
    packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`5Personal Settings active:`` `#Can customize profile``"});
    
    send_inventory_state(event);
    on::SetBux(event);
    packet::create(*event.peer, false, 0, { "OnSetPearl", 0, 1 });
    packet::create(*event.peer, false, 0, { "OnSetVouchers", 0 });
    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, pPeer->ltoken[0].c_str(), ""}); 

    {
        std::tm time = localtime();

        packet::create(*event.peer, false, 0, {
            "OnTodaysDate",
            time.tm_mon + 1,
            time.tm_mday,
            0u,
            0u
        }); 

        on::RequestWorldSelectMenu(event);
        on::RequestGazette(event);
    }

    send_data(*event.peer, compress_state(::state{ .type = 0x16 }));
}
