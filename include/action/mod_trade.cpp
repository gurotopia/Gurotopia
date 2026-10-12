#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "tools/trade.hpp"
#include "mod_trade.hpp"

void action::mod_trade(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->trade.netid == 0) return; // @note trade window isn't open

    const short id = atoi(::hPipe{ header }["itemID"].c_str());

    if (std::string why{}; !tradeable(id, why))
    {
        send_varlist(event.peer, { "OnTextOverlay", why });
        send_action(*event.peer, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
        return;
    }
    auto slot = std::ranges::find(pPeer->slots, id, &::slot::id);
    if (slot == pPeer->slots.end() || slot->count <= 0) return;

    if (slot->count == 1) // @note nothing to ask
    {
        trade_offer(event, id, 1);
        return;
    }
    const ::item &item = id_to_item(id);

    send_varlist(event.peer, {
        "OnDialogRequest",
        ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", std::format("`2Trade`` `w{}``", item.raw_name), item.id)
            .add_textbox("`2Trade how many?``")
            .add_text_input("count", "", slot->count, 5)
            .embed_data("itemID", item.id)
            .end_dialog("trade_add")
    });
}
