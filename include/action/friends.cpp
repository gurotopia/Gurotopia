#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "friends.hpp"

void action::friends(ENetEvent& event, const std::string& header) 
{
    packet::create(*event.peer, false, 0, {
        "OnDialogRequest", 
        create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", " `wSocial Portal`` ", 1366)
            .add_spacer("small")
            .add_button("showfriend", "`wShow Friends``")
            .add_button("communityhub", "`wCommunity Hub``")
            .add_button("show_apprentices", "`wShow Apprentices``")
            .add_button("showguild", "`wCreate Guild``")
            .add_button("trade_history", "`wTrade History``")
            .add_quick_exit()
            .end_dialog("socialportal", "").c_str()
    });
}