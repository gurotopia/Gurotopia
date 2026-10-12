#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/create_dialog.hpp"
#include "tools/trade.hpp"
#include "trade_confirm.hpp"

void trade_confirm(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ENetPeer *other = trade_partner(*pPeer);
    if (other == nullptr)
    {
        send_varlist(event.peer, { "OnTextOverlay", "The other person left the trade!" });
        pPeer->trade = {};
        return;
    }
    ::peer *pOthers = static_cast<::peer*>(other->data);

    /* @note "accepted" resets if either offer changed after the confirm dialog was sent */
    if (hPipe["buttonClicked"] != "accept" || !pPeer->trade.accepted || !pOthers->trade.accepted)
    {
        const std::string message = std::format("`6[```4Trade canceled by {}`4!```6]``", pPeer->display_growid);
        send_varlist(other, {
            "OnDialogRequest",
            ::create_dialog()
                .set_default_color("`o")
                .add_label_with_icon("big", "`wTrade``", 1366)
                .add_textbox(message)
                .end_dialog("trade_done", "", "OK")
        });
        trade_end(*event.peer, message);
        trade_end(*other, message);
        return;
    }
    pPeer->trade.confirmed = true;

    if (pOthers->trade.confirmed) trade_execute(*event.peer, *other);
    else on::ConsoleMessage(event.peer, "`oWaiting for the other player to confirm...``");
}
