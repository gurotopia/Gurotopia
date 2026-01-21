#include "pch.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "trash.hpp"

void action::trash(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(header, '|')[4];
    
    item &item = items[atoi(itemID.c_str())];

    if (item.cat == CAT_CANNOT_DROP)
    {
        packet::create(*event.peer, false, 0, { "OnTextOverlay", "You'd be sorry if you lost that!" });
        return;
    }
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    for (const ::slot &slot : peer->slots)
        if (slot.id == item.id)
        {
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", std::format("`4Recycle`` `w{}``", item.raw_name), slot.id)
                    .add_textbox(std::format("How many to `4destroy``? (you have {})", slot.count))
                    .add_text_input("count", "", 0, 5)
                    .embed_data("itemID", slot.id)
                    .end_dialog("trash_item").c_str()
            });
            return;
        }
}