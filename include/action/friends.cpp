#include "pch.hpp"
#include "friends.hpp"

void action::friends(ENetEvent& event, const std::string& header) 
{
    packet::create(*event.peer, false, 0, {
        "OnDialogRequest", 
        "set_default_color|`o\n"
        "add_label_with_icon|big| `wSocial Portal`` |left|1366|\n"
        "add_spacer|small|\n"
        "add_button|showfriend|`wShow Friends``|noflags|0|0|\n"
        "add_button|communityhub|`wCommunity Hub``|noflags|0|0|\n"
        "add_button|show_apprentices|`wShow Apprentices``|noflags|0|0|\n"
        "add_button|showguild|`wCreate Guild``|noflags|0|0|\n"
        "add_button|trade_history|`wTrade History``|noflags|0|0|\n"
        "add_quick_exit|\n"
        "end_dialog|socialportal||Back|\n"
    });
}