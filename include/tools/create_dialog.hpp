#pragma once
#ifndef TOOLS_CREATE_DIALOG_HPP
#define TOOLS_CREATE_DIALOG_HPP

class create_dialog
{
    std::string _d{};

public:
    create_dialog& set_default_color(std::string code);
    create_dialog& add_label(std::string size, std::string label);
    create_dialog& add_label_with_icon(std::string size, std::string label, int icon);
    create_dialog& add_label_with_ele_icon(std::string size, std::string label, int icon, u_char element);
    create_dialog& add_textbox(std::string label);
    create_dialog& add_text_input(std::string id, std::string label, short set_value, short length);
    create_dialog& embed_data(std::string id, std::string data);
    create_dialog& embed_data(std::string id, int data);
    create_dialog& add_spacer(std::string size);
    create_dialog& set_custom_spacing(short x, short y);
    create_dialog& add_button(std::string btn_id, std::string btn_name);
    create_dialog& add_custom_button(std::string btn_id, std::string image);
    create_dialog& add_custom_break();
    create_dialog& add_custom_margin(short x, short y);
    create_dialog& add_achieve(std::string total);
    create_dialog& add_quick_exit();
    create_dialog& add_popup_name(std::string popup_name);
    create_dialog& add_player_info(std::string label, std::string progress_bar_name, std::string progress, std::string total_progress);

    std::string end_dialog(std::string btn_id, std::string btn_close = "Cancel", std::string btn_return = "OK");
};

#endif