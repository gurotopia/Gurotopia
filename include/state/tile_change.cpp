#include "pch.hpp"
#include "network/packet.hpp"
#include "on/NameChanged.hpp"
#include "commands/weather.hpp"
#include "equip.hpp"
#include "tile_change.hpp"

#include "tools/randomizer.hpp"

#include <cmath>

#if defined(_WIN32) && defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif
using namespace std::literals::chrono_literals; // @note for 'ms', 's', ect.

void tile_change(ENetEvent event, state state) 
{
    try
    {
        if (not create_rt(event, 0, 160)) return;
        auto &peer = _peer[event.peer];
        world &world = worlds[peer->recent_worlds.back()];

        /* locked worlds are only accessable by owner, admin, moderator (and above) */
        if (world.owner != 00 && peer->role == role::PLAYER)
            if (peer->user_id != world.owner &&
                !std::ranges::contains(world.admin, peer->user_id)) return;

        block &block = world.blocks[cord(state.punch[0], state.punch[1])];
        item &item_fg = items[block.fg];
        item &item_id = items[state.id];

        if (state.id == 18) // @note punching a block
        {
            if (block.bg == 0 && block.fg == 0) return;
            if (item_fg.type == std::byte{ type::STRONG }) throw std::runtime_error("It's too strong to break.");
            if (item_fg.type == std::byte{ type::MAIN_DOOR }) throw std::runtime_error("(stand over and punch to use)");

            std::vector<std::pair<short, short>> im{};
            if (item_fg.type == std::byte{ type::SEED } && (steady_clock::now() - block.tick) / 1s >= item_fg.tick)
            {
                block.hits[0] = 999;
                im.emplace_back(block.fg - 1, randomizer(1, 8));
                if (!randomizer(0, 5)) im.emplace_back(block.fg, 1);
            }
            if (item_fg.type == std::byte{ type::WEATHER_MACHINE })
            {
                int remember_weather{0};
                if (!block.toggled) 
                {
                    block.toggled = true;
                    remember_weather = get_weather_id(block.fg);
                }
                else block.toggled = false;
                peers(event, PEER_SAME_WORLD, [&remember_weather](ENetPeer& p)
                {
                    gt_packet(p, false, 0, { "OnSetCurrentWeather", remember_weather });
                });
            }
            block_punched(event, state, block);
            
            short id{};
            if (block.fg != 0 && block.hits[0] >= item_fg.hits) id = block.fg, block.fg = 0;
            else if (block.bg != 0 && block.hits[1] >= items[block.bg].hits) id = block.bg, block.bg = 0;
            else return;
            block.hits = {0, 0};
            block.label = ""; // @todo
            block.toggled = false; // @todo

            if (!randomizer(0, 7)) im.emplace_back(112, 1); // @todo get real growtopia gem drop amount.
            if (item_fg.type != std::byte{ type::SEED })
            {
                if (!randomizer(0, 13)) im.emplace_back(id, 1);
                if (!randomizer(0, 9)) im.emplace_back(id + 1, 1);
            }
            /* something will drop... */
            for (auto & i : im)
                drop_visuals(event, {i.first, i.second},
                    {
                        static_cast<float>(state.punch[0]) + randomizer(0.05f, 0.1f), 
                        static_cast<float>(state.punch[1]) + randomizer(0.05f, 0.1f)
                    });
            peer->add_xp(std::trunc(1.0f + items[id].rarity / 5.0f));
        } // @note delete im, id
        else if (item_id.cloth_type != clothing::none) 
        {
            equip(event, state); // @note imitate equip
            return; 
        }
        else if (item_id.type == std::byte{ type::CONSUMEABLE }) return;
        else if (state.id == 32)
        {
            switch (item_fg.type)
            {
                case std::byte{ type::LOCK }:
                {
                    if (peer->user_id == world.owner)
                    {
                        gt_packet(*event.peer, false, 0, {
                            "OnDialogRequest",
                            std::format("set_default_color|`o\n"
                                "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                                "add_popup_name|LockEdit|\n"
                                "add_label|small|`wAccess list:``|left\n"
                                "embed_data|tilex|{}\n"
                                "embed_data|tiley|{}\n"
                                "add_spacer|small|\n"
                                "add_label|small|Currently, you're the only one with access.``|left\n"
                                "add_spacer|small|\n"
                                "add_player_picker|playerNetID|`wAdd``|\n"
                                "add_checkbox|checkbox_public|Allow anyone to Build and Break|0\n"
                                "add_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0\n"
                                "add_text_input|tempo|Music BPM|100|3|\n"
                                "add_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|0\n"
                                "add_smalltext|Your current home world is: JOLEIT|left|\n"
                                "add_checkbox|checkbox_set_as_home_world|Set as Home World|0|\n"
                                "add_text_input|minimum_entry_level|World Level: |1|3|\n"
                                "add_smalltext|Set minimum world entry level.|\n"
                                "add_button|sessionlength_dialog|`wSet World Timer``|noflags|0|0|\n"
                                "add_button|changecat|`wCategory: None``|noflags|0|0|\n"
                                "add_button|getKey|Get World Key|noflags|0|0|\n"
                                "end_dialog|lock_edit|Cancel|OK|\n",
                                item_fg.raw_name, block.fg, state.punch[0], state.punch[1]
                            ).c_str()
                        });
                    }
                    break;
                }
                case std::byte{ type::DOOR }:
                        gt_packet(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_text_input|door_name|Label|{}|100|\n"
                            "add_popup_name|DoorEdit|\n"
                            "add_text_input|door_target|Destination||24|\n"
                            "add_smalltext|Enter a Destination in this format: `2WORLDNAME:ID``|left|\n"
                            "add_smalltext|Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.|left|\n"
                            "add_text_input|door_id|ID||11|\n"
                            "add_smalltext|Set a unique `2ID`` to target this door as a Destination from another!|left|\n"
                            "add_checkbox|checkbox_locked|Is open to public|1\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|door_edit|Cancel|OK|", 
                            item_fg.raw_name, block.fg, block.label, state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
                case std::byte{ type::SIGN }:
                        gt_packet(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "add_popup_name|SignEdit|\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_textbox|What would you like to write on this sign?``|left|\n"
                            "add_text_input|sign_text||{}|128|\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|sign_edit|Cancel|OK|", 
                            item_fg.raw_name, block.fg, block.label, state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
                case std::byte{ type::ENTRANCE }:
                    gt_packet(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "set_default_color|`o"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|"
                            "add_checkbox|checkbox_public|Is open to public|1"
                            "embed_data|tilex|{}"
                            "embed_data|tiley|{}"
                            "end_dialog|gateway_edit|Cancel|OK|", 
                            item_fg.raw_name, block.fg, state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
            }
            return; // @note leave early else wrench will act as a block unlike fist which breaks. this is cause of state_visuals()
        }
        else // @note placing a block
        {
            switch (item_id.type)
            {
                case std::byte{ type::LOCK }:
                {
                    if (world.owner == 00)
                    {
                        world.owner = peer->user_id;
                        if (peer->role == role::PLAYER) peer->prefix = "2";
                        state.type = 0x0f;
                        state.netid = world.owner;
                        state.peer_state = 0x08;
                        state.id = item_id.id;
                        if (std::ranges::find(peer->my_worlds, world.name) == peer->my_worlds.end()) 
                        {
                            std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
                            peer->my_worlds.back() = world.name;
                        }
                        peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
                        {
                            std::string placed_message{ std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world.name, peer->ltoken[0]) };
                            gt_packet(p, false, 0, {
                                "OnTalkBubble", 
                                peer->netid,
                                placed_message.c_str(),
                                0u
                            });
                            gt_packet(p, false, 0, {
                                "OnConsoleMessage",
                                placed_message.c_str()
                            });
                        });
                        OnNameChanged(event);
                    }
                    else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
                    break;
                }
                case std::byte{ type::SEED }:
                case std::byte{ type::PROVIDER }:
                {
                    block.tick = steady_clock::now();
                    break;
                }
                case std::byte{ type::WEATHER_MACHINE }:
                {
                    block.toggled = true;
                    peers(event, PEER_SAME_WORLD, [&block](ENetPeer& p)
                    {
                        gt_packet(p, false, 0, { "OnSetCurrentWeather", get_weather_id(block.fg) });
                    });
                    break;
                }
            }
            if (item_id.collision == collision::full)
            {
                // 이 (left, right)
                bool x = state.punch.front() == std::lround(state.pos.front() / 32);
                // 으 (up, down)
                bool y = state.punch.back() == std::lround(state.pos.back() / 32);

                // @note because floats are rounded weirdly in Growtopia...
                bool x_nabor = state.punch.front() == std::lround(state.pos.front() / 32) + 1;
                bool y_nabor = state.punch.back() == std::lround(state.pos.back() / 32) + 1;

                bool collision = (x && y) || (x_nabor && y_nabor);
                if ((peer->facing_left && collision) || 
                    (not peer->facing_left && collision)) return;
            }
            (item_id.type == std::byte{ type::BACKGROUND }) ? block.bg = state.id : block.fg = state.id;
            peer->emplace(slot{
                static_cast<short>(state.id),
                -1 // @note remove that item the peer just placed.
            });
        }
        if (state.netid != world.owner) state.netid = peer->netid;
        state_visuals(event, std::move(state)); // finished.
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) 
            gt_packet(*event.peer, false, 0, {
                "OnTalkBubble", 
                _peer[event.peer]->netid, // @note we are not using 'peer' ref cause of ratelimit and waste of memory.
                exc.what()
            });
    }
}