#include "pch.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "wrench.hpp"

#include "tools/string_view.hpp"

#include <cmath> // @note std::round

void wrench(ENetEvent event, const std::string& header) 
{
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes[3] == "netid" && !pipes[4].empty()/*empty netid*/)
    {
        const short netid = stoi(pipes[4]);
        peers(ENET_PEER_STATE_CONNECTED, [&](ENetPeer& p) 
        {
            if (!_peer[&p]->recent_worlds.empty() && !_peer[event.peer]->recent_worlds.empty() && 
                _peer[&p]->recent_worlds.back() == _peer[event.peer]->recent_worlds.back() &&
                _peer[&p]->netid == netid)
            {
                /* wrench yourself */
                if (_peer[&p]->user_id == _peer[event.peer]->user_id)
                {
                    unsigned short lvl = _peer[&p]->level.front();
                    gt_packet(p, false, 0, {
                        "OnDialogRequest",
                        std::format(
                            "embed_data|netID|{0}\n"
                            "add_popup_name|WrenchMenu|\n"
                            "set_default_color|`o\n"
                            "add_player_info|`w{1}``|{2}|{3}|{4}|\n"
                            "add_spacer|small|\n"
                            "add_spacer|small|\n"
                            "add_button|renew_pvp_license|Get Card Battle License|noflags|0|0|\n"
                            "add_spacer|small|\n"
                            "set_custom_spacing|x:5;y:10|\n"
                            "add_custom_button|open_personlize_profile|image:interface/large/gui_wrench_personalize_profile.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|set_online_status|image:interface/large/gui_wrench_online_status_1green.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|billboard_edit|image:interface/large/gui_wrench_edit_billboard.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|notebook_edit|image:interface/large/gui_wrench_notebook.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|goals|image:interface/large/gui_wrench_goals_quests.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|bonus|image:interface/large/gui_wrench_daily_bonus_active.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|my_worlds|image:interface/large/gui_wrench_my_worlds.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|alist|image:interface/large/gui_wrench_achievements.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_label|(0/173)|target:alist;top:0.72;left:0.5;size:small|\n"
                            "add_custom_button|emojis|image:interface/large/gui_wrench_growmojis.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|marvelous_missions|image:interface/large/gui_wrench_marvelous_missions.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|title_edit|image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|trade_scan|image:interface/large/gui_wrench_trades.rttex;image_size:400,260;width:0.19;|\n"
                            "embed_data|netID|{0}\n"
                            "add_custom_button|pets|image:interface/large/gui_wrench_battle_pets.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_button|wrench_customization|image:interface/large/gui_wrench_customization.rttex;image_size:400,260;width:0.19;|\n"
                            "add_custom_break|\n"
                            "add_spacer|small|\n"
                            "set_custom_spacing|x:0;y:0|\n"
                            "add_spacer|small|\n"
                            "add_textbox|Surgeon Level: 0|left|\n"
                            "add_spacer|small|\n"
                            "add_textbox|`wActive effects:``|left|\n"
                            "{5}" // @todo add effects
                            "add_spacer|small|\n"
                            "add_textbox|`oYou have `w{6}`` backpack slots.``|left|\n"
                            "add_textbox|`oCurrent world: `w{7}`` (`w{8}``, `w{9}``) (`w0`` person)````|left|\n"
                            "add_spacer|small|\n"
                            "add_textbox|`oTotal time played is `w0.0`` hours.  This account was created `w0`` days ago.``|left|" // @todo add account creation
                            "add_spacer|small|\n"
                            "end_dialog|popup||Continue|\n"
                            "add_quick_exit|\n",
                            /* {0} {1}                   {2}  {3}                      {4}                   {5}            {6}                  */
                            netid, _peer[&p]->ltoken[0], lvl, _peer[&p]->level.back(), 50 * (lvl * lvl + 2), ""/*effects*/, _peer[&p]->slot_size, 
                            /* {4}                            {5}                            {6}                         */
                            _peer[&p]->recent_worlds.front(), std::round(_peer[&p]->pos[0]), std::round(_peer[&p]->pos[1])
                        ).c_str()
                    });
                }
                /* wrench someone else */
                else
                {

                }
                return; // @note early exit else iteration will continue for EVERYONE in the world.
            }
        });
    }
}