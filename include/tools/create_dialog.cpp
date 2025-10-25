#include <string>
#include <format>
#include <_bsd_types.h>
#include "create_dialog.hpp"


create_dialog& create_dialog::set_default_color(std::string code)
{
    _d.append(std::format("set_default_color|{}\n", code));
    return *this;
}
create_dialog& create_dialog::add_label(std::string size, std::string label)
{
    _d.append(std::format("add_label|{}|{}|left\n", size, label));
    return *this;
}
create_dialog& create_dialog::add_label_with_icon(std::string size, std::string label, int icon)
{
    _d.append(std::format("add_label_with_icon|{}|{}|left|{}|\n", size, label, icon));
    return *this;
}
create_dialog& create_dialog::add_label_with_ele_icon(std::string size, std::string label, int icon, u_char element)
{
    _d.append(std::format("add_label_with_ele_icon|{}|{}|left|{}|{}|\n", size, label, icon, element));
    return *this;
}
create_dialog& create_dialog::add_textbox(std::string label)
{
    _d.append(std::format("add_textbox|{}|left|\n", label));
    return *this;
}
create_dialog& create_dialog::add_text_input(std::string id, std::string label, short set_value, short length)
{
    _d.append(std::format("add_text_input|{}|{}|{}|{}|\n", id, label, set_value, length));
    return *this;
}
create_dialog& create_dialog::embed_data(std::string id, std::string data)
{
    _d.append(std::format("embed_data|{}|{}\n", id, data));
    return *this;
}
create_dialog& create_dialog::add_spacer(std::string size)
{
    _d.append(std::format("add_spacer|{}|\n", size));
    return *this;
}
create_dialog& create_dialog::set_custom_spacing(short x, short y)
{
    _d.append(std::format("set_custom_spacing|x:{};y:{}|\n", x, y));
    return *this;
}
create_dialog& create_dialog::add_button(std::string btn_id, std::string btn_name)
{
    _d.append(std::format("add_button|{}|{}|noflags|0|0|\n", btn_id, btn_name));
    return *this;
}
create_dialog& create_dialog::add_custom_button(std::string btn_id, std::string image)
{
    _d.append(std::format("add_custom_button|{}|{}|\n", btn_id, image));
    return *this;
}
create_dialog& create_dialog::add_custom_break()
{
    _d.append("add_custom_break|\n");
    return *this;
}
create_dialog& create_dialog::add_custom_margin(short x, short y)
{
    _d.append(std::format("add_custom_margin|x:{};y:{}|\n", x, y));
    return *this;
}
create_dialog& create_dialog::add_achieve(std::string total)
{
    _d.append(std::format("add_achieve|||left|{}|\n", total)); // @todo
    return *this;
}
create_dialog& create_dialog::add_quick_exit()
{
    _d.append("add_quick_exit|\n");
    return *this;
}
create_dialog& create_dialog::add_popup_name(std::string popup_name)
{
    _d.append(std::format("add_popup_name|{}|\n", popup_name));
    return *this;
}
create_dialog& create_dialog::add_player_info(std::string label, std::string progress_bar_name, std::string progress, std::string total_progress)
{
    _d.append(std::format("add_popup_name|{}|{}|{}|{}|\n", label, progress_bar_name, progress, total_progress));
    return *this;
}

std::string create_dialog::end_dialog(std::string btn_id, std::string btn_close, std::string btn_return) 
{ 
    _d.append(std::format("end_dialog|{}|{}|{}|\n", btn_id, btn_close, btn_return));
    return _d; 
}