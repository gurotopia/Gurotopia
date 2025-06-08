#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "action/dialog_return.hpp"

#include "tools/string_view.hpp"

void dialog_return(ENetEvent event, const std::string& header) 
{
    auto &peer = _peer[event.peer];
    std::vector<std::string> pipes = readch(header, '|');
    std::string dialog_name = pipes[3zu];
    if (pipes.size() > 3)
        pipes.erase(pipes.begin(), pipes.begin() + 4);
    else return; // if button has no name.
    if (((dialog_name == "drop_item" || dialog_name == "trash_item") && pipes[0zu] == "itemID" && pipes[3zu] == "count") && 
        (!pipes[1zu].empty() && !pipes[4zu].empty()))
    {
        const short id = stoi(pipes[1zu]);
        const short count = stoi(pipes[4zu]);
        peer->emplace(slot{id, static_cast<short>(count * -1)}); // @note take away
        inventory_visuals(event);
        if (dialog_name == "drop_item") 
        {
            float x_nabor = (peer->facing_left ? peer->pos[0] - 1 : peer->pos[0] + 1); // @note peer's naboring tile (drop position)
            drop_visuals(event, {id, count}, {x_nabor, peer->pos[1]});
        } 
        else if (dialog_name == "trash_item")
        {
            gt_packet(*event.peer, false, 0, {
                "OnConsoleMessage",
                std::format("{} `w{}`` recycled, `w0`` gems earned.", count, items[id].raw_name).c_str()
            });
        }
    }
    else if (dialog_name == "popup" && pipes[6zu] == "buttonClicked") // @todo why does netID|1| appear twice??
    {
        if (pipes[7zu] == "my_worlds")
        {
            auto section = [](const auto& range) 
            {
                std::string result;
                for (const auto& name : range)
                    if (not name.empty())
                        result += std::format("add_button|{0}|{0}|noflags|0|0|\n", name);
                return result;
            };
            gt_packet(*event.peer, false, 0, {
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
    }
    else if ((dialog_name == "find" && pipes[0zu] == "buttonClicked" && pipes[1zu].starts_with("searchableItemListButton")) && 
             !readch(pipes[1zu], '_')[1].empty())
    {
        peer->emplace(slot{static_cast<short>(stoi(readch(pipes[1zu], '_')[1])), 200});
        inventory_visuals(event);
    }
    else if ((dialog_name == "door_edit" && pipes[6zu] == "door_name") || 
             (dialog_name == "sign_edit" && pipes[6zu] == "sign_text") && 
             (!pipes[1zu].empty() || !pipes[4zu].empty()))
    {
        const short tilex = stoi(pipes[1zu]);
        const short tiley = stoi(pipes[4zu]);
        world &world = worlds[peer->recent_worlds.back()];
        block &block = world.blocks[cord(tilex, tiley)];
        block.label = pipes[7zu];

        state s{
            .id = block.fg,
            .pos = { tilex * 32.0f, tiley * 32.0f },
            .punch = { tilex, tiley }
        };
        tile_update(event, s, block, world);
    }
}