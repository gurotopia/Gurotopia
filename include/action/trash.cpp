#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "trash.hpp"

void action::trash(ENetEvent& event, const std::string& header)
{
    const std::string &itemID = readch(header, '|')[4];

    const ::item &item = id_to_item(atoi(itemID.c_str()));

    if (item.type == type::FIST || item.type == type::WRENCH)
    {
        send_varlist(event.peer, { "OnTextOverlay", "You'd be sorry if you lost that!" });
        return;
    }
    // @todo add confirm message on untradeables
    
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    for (const ::slot &slot : pPeer->slots)
        if (slot.id == item.id)
        {
            send_varlist(event.peer, {
                "OnDialogRequest",
                create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", std::format("`4Recycle`` `w{}``", item.raw_name), slot.id)
                    .add_textbox(std::format("How many to `4destroy``? (you have {})", slot.count))
                    .add_text_input("count", "", 0, 5)
                    .embed_data("itemID", slot.id)
                    .end_dialog("trash_item")
            });
            return;
        }
}