#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "inventoryfavuitrigger.hpp"

void action::inventoryfavuitrigger(ENetEvent& event, const std::string& header)
{
    packet::create(*event.peer, false, 0,  {
        "OnDialogRequest",
        create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wFavorited Items``", 13814)
            .add_spacer("small")
            .add_textbox("All favorited items are currently in your inventory. They can be unfavorited by tapping on the UNFAV button while having the item selected in your inventory.")
            .add_spacer("small")
            .end_dialog("unfavorite_items_dialog", "Close", "").c_str()
    });
}