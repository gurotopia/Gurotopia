#include "pch.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "wrench.hpp"

#include <cmath>

void action::wrench(ENetEvent& event, const std::string& header) 
{
    std::vector<std::string> pipes = readch(header, '|');
    if ((pipes[3zu] == "netid" && !pipes[4zu].empty()/*empty netid*/))
    {
        const short netid = atoi(pipes[4zu].c_str());
        peers(event, PEER_SAME_WORLD, [event, netid](ENetPeer& p) 
        {
            if (_peer[&p]->netid == netid)
            {
                auto &peer = _peer[&p];
                u_short lvl = peer->level.front();
                /* wrench yourself */
                if (peer->user_id == _peer[event.peer]->user_id)
                {
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .embed_data("netID", netid)
                            .add_popup_name("WrenchMenu")
                            .set_default_color("`o")
                            .add_player_info(std::format("`{}{}``", peer->prefix, peer->ltoken[0]), std::to_string(lvl), peer->level.back(), 50 * (lvl * lvl + 2))
                            .add_spacer("small")
                            .add_spacer("small")
                            .add_button("renew_pvp_license", "Get Card Battle License")
                            .add_spacer("small")
                            .set_custom_spacing(5, 10)
                            .add_custom_button("open_personlize_profile", "image:interface/large/gui_wrench_personalize_profile.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("set_online_status", "image:interface/large/gui_wrench_online_status_1green.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("billboard_edit", "image:interface/large/gui_wrench_edit_billboard.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("wardrobe_customization", "image:interface/large/gui_wrench_wardrobe.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("notebook_edit", "image:interface/large/gui_wrench_notebook.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("goals", "image:interface/large/gui_wrench_goals_quests.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("bonus", "image:interface/large/gui_wrench_daily_bonus_active.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("my_worlds", "image:interface/large/gui_wrench_my_worlds.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("alist", "image:interface/large/gui_wrench_achievements.rttex;image_size:400,260;width:0.19;")
                            .add_custom_label("(0/173)"/*@todo add achivements*/, "target:alist;top:0.72;left:0.5;size:small")
                            .add_custom_button("emojis", "image:interface/large/gui_wrench_growmojis.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("marvelous_missions", "image:interface/large/gui_wrench_marvelous_missions.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("title_edit", "image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("trade_scan", "image:interface/large/gui_wrench_trades.rttex;image_size:400,260;width:0.19;")
                            .embed_data("netID", netid) // @todo research why rgt adds this twice...
                            .add_custom_button("pets", "image:interface/large/gui_wrench_battle_pets.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("wrench_customization", "image:interface/large/gui_wrench_customization.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("open_worldlock_storage", "image:interface/large/gui_wrench_auction.rttex;image_size:400,260;width:0.19;")
                            .add_custom_break()
                            .add_spacer("small")
                            .set_custom_spacing(0, 0) // @todo research why rgt adds this too. is 0, 0 unaffecting? or is this resetting the previous 5, 10 spacing
                            .add_spacer("small")
                            .add_textbox("Surgeon Level: 0")
                            .add_spacer("small")
                            .add_textbox("`wActive effects:``")
                            /* @todo handle peer's effects */
                            .add_spacer("small")
                            .add_textbox(std::format("`oYou have `w{}`` backpack slots.``", peer->slot_size))
                            .add_textbox(std::format("`oCurrent world: `w{}`` (`w{}``, `w{}``) (`w0`` person)````", peer->recent_worlds.back(), std::round(peer->pos[0]), std::round(peer->pos[1])))
                            .add_spacer("small")
                            .add_textbox("`oTotal time played is `w0.0`` hours.  This account was created `w0`` days ago.``")
                            .add_spacer("small")
                            .add_quick_exit()
                            .end_dialog("popup", "", "Continue").c_str()
                    });
                }
                /* wrench someone else */
                else
                {
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .embed_data("netID", netid)
                            .add_popup_name("WrenchMenu")
                            .set_default_color("`o")
                            .add_label_with_icon("big", std::format("`{}{} (`2{}``)``", peer->prefix, peer->ltoken[0], lvl), 18)
                            .embed_data("netID", netid)
                            .add_spacer("small")
                            .add_achieve("0"/*@todo add achivements*/)
                            .add_custom_margin(75, -70.85)
                            .add_custom_margin(-75, 70.85)
                            .add_spacer("small")
                            .add_label("small", "`1Achievements:`` 0/173"/*add total achivements*/)
                            .add_spacer("small")
                            .add_label("small", "`1Account Age:`` 0 days")
                            .add_spacer("small")
                            .add_button("trade", "`wTrade``")
                            .add_button("sendpm", "`wSend Message``")
                            .add_textbox("(No Battle Leash equipped)")
                            .add_textbox("You need a valid license to battle!")
                            .add_button("friend_add", "`wAdd as friend``")
                            .add_button("show_clothes", "`wView worn clothes``")
                            .add_button("ignore_player", "`wIgnore Player``")
                            .add_button("report_player", "`wReport Player``")
                            .add_spacer("small")
                            .add_quick_exit()
                            .end_dialog("popup", "", "Continue").c_str()
                    });
                }
                return; // @note early exit else iteration will continue for EVERYONE in the world.
            }
        });
    }
}