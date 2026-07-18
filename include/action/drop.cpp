#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "drop.hpp"

void action::drop(ENetEvent& event, const std::string& header)
{
    const std::string &itemID = readch(header, '|')[4];
    
    const ::item &item = id_to_item(atoi(itemID.c_str()));
    
    if (item.cat == CAT_UNTRADEABLE)
    {
        send_varlist(event.peer, { "OnTextOverlay", "You can't drop that." });
        return;
    }
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    for (const ::slot &slot : pPeer->slots)
        if (slot.id == item.id) 
        {
            send_varlist(event.peer, { 
                "OnDialogRequest", 
                create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", std::format("`wDrop {}``", item.raw_name), item.id)
                    .add_textbox("How many to drop?")
                    .add_text_input("count", "", slot.count, 5)
                    .embed_data("itemID", item.id)
                    .end_dialog("drop_item")
            });
            return;
        }
}