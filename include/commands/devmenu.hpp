#pragma once

extern std::string_view help_for(int level); // @note 0 everyone, 1 mod, 2 dev (from __command.cpp)

extern void devmenu_open(ENetEvent &event);
extern bool devmenu_form(ENetEvent &event, const std::string &name, bool only_if_required);
extern void dev_menu_return(ENetEvent &event, const ::hPipe &hPipe);
extern void cmd_form_return(ENetEvent &event, const ::hPipe &hPipe);

extern void find_item_popup(ENetEvent &event, int id);
extern void find_give_return(ENetEvent &event, const ::hPipe &hPipe);
