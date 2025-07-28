#include "pch.hpp"
#include "on/BillboardChange.hpp"
#include "on/SetBux.hpp"
#include "on/NameChanged.hpp"
#include "tools/string.hpp"
#include "quit_to_exit.hpp"
#include "join_request.hpp"
#include "action/dialog_return.hpp"

void action::dialog_return(ENetEvent& event, const std::string& header) 
{
    auto &peer = _peer[event.peer];
    std::vector<std::string> pipes = readch(std::move(header), '|');

    if (pipes.size() <= 3) return; // if button has no name or has no field.
    
    if (((pipes[3zu] == "drop_item" || pipes[3zu] == "trash_item") && pipes[4zu] == "itemID" && pipes[7zu] == "count") && 
        (number(pipes[5zu]) && number(pipes[8zu])))
    {
        const short id = stoi(pipes[5zu]);
        const short count = stoi(pipes[8zu]);
        peer->emplace(slot(id, -count)); // @note take away
        inventory_visuals(event);
        if (pipes[3zu] == "drop_item") 
        {
            float x_nabor = (peer->facing_left ? peer->pos[0] - 1 : peer->pos[0] + 1); // @note peer's naboring tile (drop position)
            item_change_object(event, {id, count}, {x_nabor, peer->pos[1]});
        } 
        else if (pipes[3zu] == "trash_item")
        {
            packet::create(*event.peer, false, 0, {
                "OnConsoleMessage",
                std::format("{} `w{}`` recycled, `w0`` gems earned.", count, items[id].raw_name).c_str()
            });
        }
    }
    else if (pipes[3zu] == "popup" && pipes[10zu] == "buttonClicked") // @todo why does netID|1| appear twice??
    {
        if (pipes[11zu] == "my_worlds")
        {
            auto section = [](const auto& range) 
            {
                std::string result;
                for (const auto& name : range)
                    if (!name.empty())
                        result.append(std::format("add_button|{0}|{0}|noflags|0|0|\n", name));
                return result;
            };
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format(
                    "set_default_color|`o\n"
                    "start_custom_tabs|\n"
                    "add_custom_button|myWorldsUiTab_0|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:1,0;width:0.15;|\n"
                    "add_custom_button|myWorldsUiTab_1|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,1;width:0.15;|\n"
                    "add_custom_button|myWorldsUiTab_2|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,2;width:0.15;|\n"
                    "end_custom_tabs|\n"
                    "add_label|big|Locked Worlds|left|0|\n"
                    "add_spacer|small|\n"
                    "add_textbox|Place a World Lock in a world to lock it. Break your World Lock to unlock a world.|left|\n"
                    "add_spacer|small|\n"
                    "{}\n"
                    "add_spacer|small|\n"
                    "end_dialog|worlds_list||Back|\n"
                    "add_quick_exit|\n",
                    section(peer->my_worlds)
                ).c_str()
            });
        }
        else if (pipes[11zu] == "billboard_edit")
        {
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format("set_default_color|`o\n"
                "add_label_with_icon|big|`wTrade Billboard``|left|8282|\n"
                "add_spacer|small|\n"
                "{}"
                "add_item_picker|billboard_item|`wSelect Billboard Item``|Choose an item to put on your billboard!|\n"
                "add_spacer|small|\n"
                "add_checkbox|billboard_toggle|`$Show Billboard``|{}\n"
                "add_checkbox|billboard_buying_toggle|`$Is Buying``|{}\n"
                "add_text_input|setprice|Price of item:|{}|5|\n"
                "add_checkbox|chk_peritem|World Locks per Item|{}\n"
                "add_checkbox|chk_perlock|Items per World Lock|{}\n"
                "add_spacer|small|\n"
                "end_dialog|billboard_edit|Close|Update|\n",
                (peer->billboard.id == 0) ? 
                    "" : 
                    std::format("add_label_with_icon|small|`w{}``|left|{}|\n", items[peer->billboard.id].raw_name, peer->billboard.id),
                to_char(peer->billboard.show),
                to_char(peer->billboard.isBuying),
                peer->billboard.price,
                to_char(peer->billboard.perItem),
                to_char(!peer->billboard.perItem)
                ).c_str()
            });
        }
    }
    else if (pipes[3zu] == "find" && pipes[4zu] == "buttonClicked" && pipes[5zu].starts_with("searchableItemListButton"))
    {
        std::string id = readch(std::move(pipes[5zu]), '_')[1];
        if (!number(id)) return;
        
        peer->emplace(slot(stoi(id), 200));
        inventory_visuals(event);
    }
    else if (pipes[3zu] == "gateway_edit" || (pipes[3zu] == "door_edit" && pipes[10zu] == "door_name") || 
             (pipes[3zu] == "sign_edit" && pipes[10zu] == "sign_text") && 
             (number(pipes[5zu]) && number(pipes[8zu])))
    {
        const short tilex = stoi(pipes[5zu]);
        const short tiley = stoi(pipes[8zu]);
        auto it = worlds.find(peer->recent_worlds.back());
        if (it == worlds.end()) return;
        block &block = it->second.blocks[cord(tilex, tiley)];

        if (pipes[3zu] == "door_edit" || pipes[3zu] == "sign_edit") block.label = pipes[11zu];
        else if (pipes[3zu] == "gateway_edit") block._public = stoi(pipes[11zu]);

        state s{
            .id = block.fg,
            .pos = { tilex * 32.0f, tiley * 32.0f },
            .punch = { tilex, tiley }
        };
        tile_update(event, s, block, it->second);

        if (pipes[10zu] == "door_name" && pipes.size() > 12zu)
        {
            for (::door& door : it->second.doors)
            {
                if (door.pos == std::array<int, 2ULL>{ tilex, tiley }) 
                {
                    door.dest = pipes[13];
                    door.id = pipes[15];
                    return;
                }
            }
            it->second.doors.emplace_back(door(
                pipes[13],
                pipes[15],
                "", // @todo add password door
                { tilex, tiley }
            ));
        }
    }
    else if (pipes[3zu] == "billboard_edit" && !pipes[5zu].empty())
    {
        if (pipes[4zu] == "billboard_item") 
        {
            const short id = stoi(pipes[5zu]); // @note this is billboard_item, I will use it a lot so I shorten to "id"
            if (id == 18 || id == 32) return;
            peer->billboard = {
                .id = id,
                .show = stoi(pipes[7zu]) != 0,
                .isBuying = stoi(pipes[9zu]) != 0,
                .price = stoi(pipes[11zu]),
                .perItem = stoi(pipes[13zu]) != 0,
            };
        }
        else // @note billboard_toggle
        {
            peer->billboard = {
                .id = peer->billboard.id,
                .show = stoi(pipes[5zu]) != 0,
                .isBuying = stoi(pipes[7zu]) != 0,
                .price = stoi(pipes[9zu]),
                .perItem = stoi(pipes[11zu]) != 0,
            };
        }
        on::BillboardChange(event);
    }
    else if (pipes[3zu] == "lock_edit")
    {
        if (pipes[10] == "checkbox_public" && pipes[11] == "1"/*true*/ || pipes[11] == "0"/*false*/)
        {
            auto it = worlds.find(peer->recent_worlds.back());
            if (it == worlds.end()) return;
            it->second._public = stoi(pipes[11]);

            // @todo add public lock visuals
        }
    }
    else if (pipes[3zu] == "blast")
    {
        const u_short id = stoi(pipes[5zu]);
        std::string world_name = pipes[8zu];
        std::for_each(world_name.begin(), world_name.end(), [](char& c) { c = std::toupper(c); });
        switch (id)
        {
            case 1402: // @note Thermonuclear Blast
            {
                auto [it, inserted] = worlds.try_emplace(world_name, world_name);
                world &world = it->second;
                if (world.name.empty())
                {
                    action::quit_to_exit(event, "", true);
                    blast::thermonuclear(world, world_name);
                    action::join_request(event, "", world_name);
                }
                break;
            }
        }
    }
    else if (pipes[3zu] == "socialportal")
    {
        /* buttonClicked */
        if (pipes[5zu] == "showfriend")
        {
            /* temp data */
            u_char __online{};

            // @todo improve!! cause I hate this so much t-t
            for (Friend &f : peer->friends)
                peers(event, PEER_ALL, [&f, &__online](ENetPeer& peer){
                    auto &p = _peer[&peer];
                    if (p->ltoken[0] == f.name)
                    {
                        ++__online;
                    }
                });

            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format(
                    "set_default_color|`o\n"
                    "add_label_with_icon|big|0 of {} `wFriends Online``|left|1366|\n"
                    "add_spacer|small|\n"
                    "add_textbox|`oNone of your friends are currently online.``|left|\n"
                    "add_spacer|small|\n"
                    "add_spacer|small|\n"
                    "add_button|friend_all|Show offline and ignored too|noflags|0|0|\n"
                    "add_button|all_friends|Edit Friends|noflags|0|0|\n"
                    "add_button|friends_options|Friend Options|noflags|0|0|\n"
                    "add_button|back|Back|noflags|0|0|\n"
                    "add_button||Close|noflags|0|0|\n"
                    "end_dialog|friends|||\n"
                    "add_quick_exit|\n",
                    __online
                ).c_str()
            });
        }
    }
    else if (pipes[3zu] == "peer_edit")
    {
        const std::string name = pipes[5zu];
        const bool status = stoi(pipes[8zu]);

        const u_char role = stoi(pipes[11zu]);
        const short level = stoi(pipes[13zu]);
        const signed gems = stoi(pipes[15zu]);

        if (status) // @note online
        {
            peers(event, PEER_ALL, [&event, name, role, level, gems](ENetPeer& p) 
            {
                if (_peer[&p]->ltoken[0] == name)
                {
                    auto &_p = _peer[&p];
                    _p->role = role;
                    _p->level[0] = level;
                    _p->gems = gems;

                    on::NameChanged(event);
                    on::SetBux(event);
                    return;
                }
            });
        }
        else // @note offline
        {
            ::peer &offline = ::peer().read(name);
            offline.role = role;
            offline.level[0] = level;
            offline.gems = gems;
        }
    }
}