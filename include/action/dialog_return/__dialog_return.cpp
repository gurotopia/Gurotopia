#include "pch.hpp"

#include "popup.hpp"
#include "drop_item.hpp"
#include "trash_item.hpp"
#include "find_item.hpp"
#include "gateway_edit.hpp"
#include "billboard_edit.hpp"
#include "lock_edit.hpp"
#include "create_blast.hpp"
#include "socialportal.hpp"
#include "megaphone.hpp"
#include "commands/worldtools.hpp"
#include "commands/devmenu.hpp"
#include "commands/growscan.hpp"
#include "commands/jammers.hpp"
#include "commands/accounts.hpp"
#include "commands/staff.hpp"
#include "nuke_confirm.hpp"
#include "commands/cooking.hpp"
#include "commands/trade.hpp"
#include "commands/friendsys.hpp"
#include "commands/legendary.hpp"

#include "__dialog_return.hpp"

std::unordered_map<std::string, std::function<void(ENetEvent &, const ::hPipe &)>> dialog_return_pool
{
    {"popup", std::bind(&popup, std::placeholders::_1, std::placeholders::_2)},

    {"drop_item", std::bind(&drop_item, std::placeholders::_1, std::placeholders::_2)},
    {"trash_item", std::bind(&trash_item, std::placeholders::_1, std::placeholders::_2)},
    {"find_item", std::bind(&find_item, std::placeholders::_1, std::placeholders::_2)},

    {"gateway_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},
    {"door_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},
    {"sign_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},

    {"billboard_edit", std::bind(&billboard_edit, std::placeholders::_1, std::placeholders::_2)},
    {"lock_edit", std::bind(&lock_edit, std::placeholders::_1, std::placeholders::_2)},

    {"create_blast", std::bind(&create_blast, std::placeholders::_1, std::placeholders::_2)},
    {"socialportal", std::bind(&socialportal, std::placeholders::_1, std::placeholders::_2)},
    {"megaphone", std::bind(&megaphone, std::placeholders::_1, std::placeholders::_2)},
    {"birth_certificate", std::bind(&birth_certificate_return, std::placeholders::_1, std::placeholders::_2)},
    {"change_address", std::bind(&change_address_return, std::placeholders::_1, std::placeholders::_2)},
    {"delete_all_worlds", std::bind(&delete_all_worlds_return, std::placeholders::_1, std::placeholders::_2)},
    {"fill_world", std::bind(&fill_world_return, std::placeholders::_1, std::placeholders::_2)},
    {"dev_menu", std::bind(&dev_menu_return, std::placeholders::_1, std::placeholders::_2)},
    {"cmd_form", std::bind(&cmd_form_return, std::placeholders::_1, std::placeholders::_2)},
    {"find_give", std::bind(&find_give_return, std::placeholders::_1, std::placeholders::_2)},
    {"growscan", std::bind(&growscan_return, std::placeholders::_1, std::placeholders::_2)},
    {"jammer_edit", std::bind(&jammer_edit_return, std::placeholders::_1, std::placeholders::_2)},
    {"accounts", std::bind(&accounts_return, std::placeholders::_1, std::placeholders::_2)},
    {"trade_add", std::bind(&trade_return, std::placeholders::_1, std::placeholders::_2)},
    {"trade_confirm", std::bind(&trade_return, std::placeholders::_1, std::placeholders::_2)},
    {"legendary_wizard", std::bind(&wizard_return, std::placeholders::_1, std::placeholders::_2)},
    {"title_edit", std::bind(&titles_return, std::placeholders::_1, std::placeholders::_2)},
    {"friend_request", std::bind(&friends_return, std::placeholders::_1, std::placeholders::_2)},
    {"friends_list", std::bind(&friends_return, std::placeholders::_1, std::placeholders::_2)},
    {"nuke_confirm", std::bind(&nuke_confirm, std::placeholders::_1, std::placeholders::_2)},
    {"cooking_dialog", std::bind(&cooking_dialog, std::placeholders::_1, std::placeholders::_2)},
};