#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "tools/trade.hpp"
#include "trade_accept.hpp"

/* @brief show both "accept" ticks in both trade windows */
static void send_accepts(ENetPeer &peer, ENetPeer &other)
{
    for (ENetPeer *to : { &peer, &other })
        for (ENetPeer *owner : { &peer, &other })
        {
            const ::peer *pOwner = static_cast<::peer*>(owner->data);
            trade_status(*to, *pOwner, std::format("\nlocked|1\naccepted|{}\n", to_char(pOwner->trade.accepted)));
        }
}

/*
    @brief check that pPeer's backpack can hold `get` after giving away their own offer
    @param too_many the item that would go over 200, 0 if the backpack is out of slots instead
*/
static bool fits(const ::peer &pPeer, const std::vector<::slot> &get, short &too_many)
{
    auto count_of = [](const std::vector<::slot> &slots, short id) -> int
    {
        auto it = std::ranges::find(slots, id, &::slot::id);
        return (it == slots.end()) ? 0 : it->count;
    };
    int used = std::ranges::count_if(pPeer.slots, [](const ::slot &slot) { return slot.count > 0; });

    for (const ::slot &slot : pPeer.trade.offer)
        if (count_of(pPeer.slots, slot.id) - slot.count <= 0) --used; // @note gave the whole stack away

    for (const ::slot &slot : get)
    {
        const int have = count_of(pPeer.slots, slot.id) - count_of(pPeer.trade.offer, slot.id);
        if (have + slot.count > 200)
        {
            too_many = slot.id;
            return false;
        }
        if (have <= 0) ++used; // @note needs a new slot
    }
    too_many = 0;
    return used <= pPeer.slot_size;
}

static std::string confirm_dialog(const std::vector<::slot> &give, const std::vector<::slot> &get)
{
    ::create_dialog dialog{};
    auto list = [&dialog](const std::vector<::slot> &offer)
    {
        if (offer.empty()) dialog.add_textbox("`4Nothing!``");
        for (const ::slot &slot : offer)
            dialog.add_label_with_icon("small", std::format("(`w{}``) {}", slot.count, id_to_item(slot.id).raw_name), slot.id);
    };
    dialog.set_default_color("`o")
        .add_label_with_icon("big", "`wTrade Confirmation``", 1366)
        .add_spacer("small")
        .add_textbox("`4You'll give:``")
        .add_spacer("small");
    list(give);
    dialog.add_spacer("small")
        .add_textbox("`2You'll get:``")
        .add_spacer("small");
    list(get);

    if (get.empty())
        dialog.add_spacer("small")
            .add_textbox("`4SCAM WARNING: ``You are about to do a trade without receiving anything in return. Once you do the trade you cannot get the items back.")
            .add_textbox("`4Do you really want to do this?``");

    return dialog.add_spacer("small")
        .add_button("accept", "Do The Trade!")
        .add_button("back", "Cancel")
        .end_dialog("trade_confirm", "", "");
}

void action::trade_accept(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ENetPeer *other = trade_partner(*pPeer);
    if (other == nullptr) return;
    ::peer *pOthers = static_cast<::peer*>(other->data);

    pPeer->trade.accepted = ::hPipe{ header }["status"] == "1";
    send_accepts(*event.peer, *other);
    if (!pPeer->trade.accepted || !pOthers->trade.accepted) return;

    /* both accepted, check both backpacks first */
    for (const auto &[pTrader, pReceiver] : { std::pair{ pPeer, pOthers }, std::pair{ pOthers, pPeer } })
    {
        short too_many{};
        if (fits(*pReceiver, pTrader->trade.offer, too_many)) continue;

        const std::string message = (too_many != 0) ?
            std::format("`4Oops - {} `4is carrying too many {} and can't fit that many in their backpack.", pReceiver->display_growid, id_to_item(too_many).raw_name) :
            std::format("{} `wneeds more backpack room first!", pReceiver->display_growid);

        send_varlist(event.peer, { "OnTextOverlay", message });
        send_varlist(other, { "OnTextOverlay", message });
        pPeer->trade.accepted = pOthers->trade.accepted = false;
        send_accepts(*event.peer, *other);
        return;
    }

    /* last step: both see what they give and get */
    send_varlist(event.peer, { "OnForceTradeEnd" });
    send_varlist(other, { "OnForceTradeEnd" });
    send_varlist(event.peer, { "OnDialogRequest", confirm_dialog(pPeer->trade.offer, pOthers->trade.offer) });
    send_varlist(other, { "OnDialogRequest", confirm_dialog(pOthers->trade.offer, pPeer->trade.offer) });
}
