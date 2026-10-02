#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "commands/curse.hpp"
#include "commands/useitems.hpp"
#include "commands/buffs.hpp"
#include "commands/staff.hpp"
#include "commands/legendary.hpp"
#include "wrench.hpp"

void action::wrench(ENetEvent& event, const std::string& header) 
{
    std::vector<std::string> pipes = readch(header, '|');
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if ((pipes[3ull] == "netid" && !pipes[4ull].empty()/*empty netid*/))
    {
        const short netid = atoi(pipes[4ull].c_str());
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [event, pPeer, netid](ENetPeer& peer) 
        {
            ::peer *pOthers = static_cast<::peer*>(peer.data);
            if (pOthers->netid == netid)
            {
                u_short lvl = pOthers->level.front();
                /* wrench yourself */
                if (pOthers->user_id == pPeer->user_id)
                {
                    send_varlist(event.peer, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .embed_data("netID", netid)
                            .add_popup_name("WrenchMenu")
                            .set_default_color("`o")
                            .add_player_info(pOthers->display_growid, (pOthers->role >= MODERATOR ? std::string{ "?" } : std::to_string(lvl)), pOthers->level.back(), 50 * (lvl * lvl + 2))
                            .add_spacer("small")
                            .add_spacer("small")
                            .add_raw(title_quick_button(*pOthers)) // @note one-tap title on/off
                            .add_button("renew_pvp_license", "Get Card Battle License")
                            .add_spacer("small")
                            .set_custom_spacing(5, 10)
                            .add_custom_button("open_personlize_profile", "image:interface/large/gui_wrench_personalize_profile.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("set_online_status", "image:interface/large/gui_wrench_online_status_1green.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("billboard_edit", "image:interface/large/gui_wrench_edit_billboard.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("wardrobe_customization", "image:interface/large/gui_wrench_wardrobe.rttex;image_size:400,260;width:0.19;")
                            .add_custom_button("seed_diary_customization", "image:interface/large/gui_wrench_seed_diary.rttex;image_size:400,260;width:0.19;")
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
                            .embed_data("netID", netid) // @todo research why rgt adds this twice···
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
                            .add_raw(curse_wrench_line(*pOthers))
                            .add_raw(item_effects_wrench(*pOthers))
                            .add_raw(buffs_wrench(*pOthers))
                            .add_raw(mods_wrench(*pOthers))
                            /* @todo handle peer's effects */
                            .add_spacer("small")
                            .add_smalltext(std::format("Fires Put Out: {}", pOthers->fires_removed))
                            .add_spacer("small")
                            .add_textbox(std::format("`oYou have `w{}`` backpack slots.``", pOthers->slot_size))
                            .add_textbox(std::format("`oCurrent world: `w{}`` (`w{}``, `w{}``) (`w0`` person)````", 
                                                    pOthers->recent_worlds.back(), pOthers->pos.by_32(true).x_int(), pOthers->pos.by_32(true).y_int()))
                            .add_textbox("`oYou are standing on the note \"A\".``")
                            .add_spacer("small")
                            .add_textbox(std::format("`oThis account was created `w{}`` days ago.``", pOthers->created_at > 0 ? (std::time(nullptr) - pOthers->created_at) / 86400 : 0))
                            .add_spacer("small")
                            .add_quick_exit()
                            .end_dialog("popup", "", "Continue")
                    });
                }
                /* wrench someone else */
                else
                {
                    const bool staff_target = pOthers->role >= MODERATOR;
                    const bool viewer_staff = pPeer->role >= MODERATOR;
                    const std::string level_text = staff_target ? std::string{ "?" } : std::to_string(lvl);
                    const std::string age_text = (staff_target && !viewer_staff) ? std::string{ "?" } :
                        std::format("{} days", pOthers->created_at > 0 ? (std::time(nullptr) - pOthers->created_at) / 86400 : 0);

                    ::create_dialog d{};
                    d.embed_data("netID", netid)
                     .add_popup_name("WrenchMenu")
                     .set_default_color("`o")
                     .add_label_with_icon("big", std::format("{} (`2{}``)``", pOthers->display_growid, level_text), 18)
                     .embed_data("netID", netid)
                     .add_spacer("small")
                     .add_label("small", std::format("`1Account Age:`` {}", age_text))
                     .add_spacer("small")
                     .add_button("trade", "`wTrade``")
                     .add_button("sendpm", "`wSend Message``")
                     .add_button("show_clothes", "`wView worn clothes``");
                    if (!staff_target || viewer_staff) // @note players can't friend, ignore or report staff
                    {
                        d.add_button("friend_add", "`wAdd as friend``")
                         .add_button("ignore_player", "`wIgnore Player``")
                         .add_button("report_player", "`wReport Player``");
                    }
                    {
                        auto here = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
                        const bool is_owner = here != worlds.end() && here->owner != 0 && here->owner == pPeer->user_id;
                        if ((viewer_staff || is_owner) && !(staff_target && pOthers->role >= pPeer->role)) d.add_button("pull_player", "`wPull``"); // @note world owners and staff
                    }
                    if (viewer_staff) d.add_button("staff_panel", "`4Staff / Punish``"); // @note opens the staff window
                    d.add_spacer("small")
                     .add_quick_exit();
                    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("popup", "", "Continue") });
                }
                return; // @note early exit else iteration will continue for EVERYONE in the world.
            }
        });
    }
}