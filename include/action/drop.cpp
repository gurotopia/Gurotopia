#include "pch.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "drop.hpp"

void action::drop(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(header, '|')[4];
    
    auto item = std::ranges::find(items, atoi(itemID.c_str()), &::item::id);

    if (item->cat == CAT_UNTRADEABLE)
    {
        packet::create(*event.peer, false, 0, { "OnTextOverlay", "You can't drop that." });
        return;
    }
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    for (const ::slot &slot : peer->slots)
        if (slot.id == item->id) 
        {
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest", 
                create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", std::format("`wDrop {}``", item->raw_name), item->id)
                    .add_textbox("How many to drop?")
                    .add_text_input("count", "", slot.count, 5)
                    .embed_data("itemID", item->id)
                    .end_dialog("drop_item").c_str() // @todo handle c_str(); make packet accept std::string.
            });
            return;
        }
}