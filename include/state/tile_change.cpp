#include "pch.hpp"
#include "on/NameChanged.hpp"
#include "on/SetClothing.hpp"
#include "commands/weather.hpp"
#include "item_activate.hpp"
#include "tools/ransuu.hpp"
#include "tools/string.hpp"
#include "action/quit_to_exit.hpp"
#include "action/join_request.hpp"
#include "item_activate_object.hpp"
#include "tile_change.hpp"

#include <cmath>

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif
using namespace std::literals::chrono_literals;

void tile_change(ENetEvent& event, state state) 
{
    try
    {
        auto &peer = _peer[event.peer];
        auto w = worlds.find(peer->recent_worlds.back());
        if (w == worlds.end()) return;

        if ((w->second.owner && !w->second._public && !peer->role) &&
            (peer->user_id != w->second.owner && !std::ranges::contains(w->second.admin, peer->user_id))) return;

        block &block = w->second.blocks[cord(state.punch[0], state.punch[1])];
        item &item = (state.id != 32 && state.id != 18) ? items[state.id] : (block.fg != 0) ? items[block.fg] : items[block.bg];
        if (item.id == 0) return;
        
        if (state.id == 18) // @note punching a block
        {
            std::vector<std::pair<short, short>> im{}; // @note list of dropped items
            ransuu ransuu;
            bool _bypass{}; // @todo remove lazy method

            switch (item.id)
            {
                case 758: // @note Roulette Wheel
                {
                    u_short number = ransuu[{0, 36}];
                    char color = (number == 0) ? '2' : (ransuu[{0, 3}] < 2) ? 'b' : '4';
                    const char* message = std::format("[`{}{}`` spun the wheel and got `{}{}``!]", peer->prefix, peer->ltoken[0], color, number).c_str();
                    peers(event, PEER_SAME_WORLD, [peer, message](ENetPeer& p)
                    {
                        packet::create(p, false, 2000, { "OnTalkBubble", peer->netid, message });
                        packet::create(p, false, 2000, { "OnConsoleMessage", message });
                    });
                    break;
                }
            }
            switch (item.type)
            {
                case type::STRONG: throw std::runtime_error("It's too strong to break."); break;
                case type::MAIN_DOOR: throw std::runtime_error("(stand over and punch to use)"); break;
                case type::LOCK:
                {
                    if (peer->user_id != w->second.owner) 
                    {
                        // @todo add message saying who owns the lock.
                        return;
                    }
                    break;
                }
                case type::PROVIDER:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item.tick) // @todo limit this check.
                    {
                        switch (item.id)
                        {
                            case 1008: // @note ATM
                            {
                                /* @todo merge gems more effectively */
                                int gems = ransuu[{1, 100}]; // @note source: https://growtopia.fandom.com/wiki/ATM_Machine
                                for (int i : {100, 50, 10, 5, 1}/* gem type */)
                                    for (; gems >= i; gems -= i/* downgrade type */)
                                        im.emplace_back(112, i);
                                        
                                break;
                            }
                            case 872:/*chicken*/ case 866:/*cow*/ case 1632:/*coffee maker*/ case 3888:/*sheep*/
                            {
                                im.emplace_back(item.id+2, ransuu[{1, 2}]);
                                break;
                            }
                            case 5116:/*tea set*/
                            {
                                im.emplace_back(item.id-2, ransuu[{1, 2}]);
                                break;
                            }
                            case 2798:/*well*/
                            {
                                im.emplace_back(822/*water bucket*/, ransuu[{1, 2}]);
                                break;
                            }
                            case 928:/*science station*/ // @note source: https://growtopia.fandom.com/wiki/Science_Station
                            {
                                short chemcial = 
                                    (!ransuu[{0, 16}]) ? chemcial = 918/*P*/ : 
                                    (!ransuu[{0, 8}])  ? chemcial = 920/*B*/ : 
                                    (!ransuu[{0, 6}])  ? chemcial = 924/*Y*/ : 
                                    (!ransuu[{0, 4}])  ? chemcial = 916/*R*/ : chemcial = 914/*G*/;
                                im.emplace_back(chemcial, 1);
                                break;
                            }
                        }
                        block.tick = steady_clock::now();
                        tile_update(event, std::move(state), block, w->second); // @note update countdown on provider.
                        _bypass = true; // @todo remove lazy method
                    }
                    break;
                }
                case type::SEED:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item.tick) // @todo limit this check.
                    {
                        block.hits[0] = 999;
                        im.emplace_back(item.id - 1, ransuu[{0, 8}]); // @note fruit (from tree)
                    }
                    break;
                }
                case type::WEATHER_MACHINE:
                {
                    block.toggled = (block.toggled) ? false : true;
                    peers(event, PEER_SAME_WORLD, [block, item](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", (block.toggled) ? get_weather_id(item.id) : 0 });
                    });
                    for (::block &b : w->second.blocks)
                        if (items[b.fg]/*@todo*/.type == type::WEATHER_MACHINE && b.fg != block.fg) b.toggled = false;
                    
                    break;
                }
                case type::TOGGLEABLE_BLOCK:
                case type::TOGGLEABLE_ANIMATED_BLOCK:
                {
                    block.toggled = (block.toggled) ? false : true;
                    if (item.id == 226) // @note Signal Jammer
                    {
                        packet::create(*event.peer, false, 0, {
                            "OnConsoleMessage",
                            (block.toggled) ? "Signal jammer enabled. This world is now `4hidden`` from the universe." :
                                              "Signal jammer disabled.  This world is `2visible`` to the universe."
                        });
                    }
                    break;
                }
            }
            tile_apply_damage(event, std::move(state), block);
            
            short remember_id = item.id;
            if (_bypass) goto skip_reset_tile; // @todo remove lazy method

            if (block.hits.front() >= item.hits) block.fg = 0, block.hits.front() = 0;
            else if (block.hits.back() >= item.hits) block.bg = 0, block.hits.back() = 0;
            else return;
            
            /* @todo update these changes with tile_update() */
            block.label = "";
            block.toggled = false; // @todo handle background toggles, if any.
            block.state3 = 0x00; // @note reset tile direction
            block.state4 &= ~S_VANISH; // @note remove paint

            if (item.type == type::LOCK) 
            {
                peer->prefix.front() = 'w';
                w->second.owner = 0; // @todo handle sl, bl, hl
            }

            if (item.cat == 0x02) // pick up (item goes back in your inventory)
            {
                int uid = item_change_object(event, {remember_id, 1}, state.pos);
                item_activate_object(event, ::state{.id = uid});
            }
            else if (u_char(item.property) & 04) { } // @note "This item never drops any seeds."; should it drop a block?
            else // @note normal break (drop gem, seed, block & give XP)
            {
                { /* gem drop */
                    std::unordered_map<u_int, int> farmables { // @todo remove hardcode
                        { 340, 22 }, // @note Chandelier
                        { 454, 11 }, // @note Venus Guytrap
                        { 526, 14 }, // @note Pinball Bumper
                        { 596, 14 }, // @note Treasure Chest
                        { 3002, 11}, // @note Fish Tank
                        { 3838, 22}, // @note Sorcerer Stone
                        { 5666, 18} // @note Laser Grid
                    };
                    if (farmables.contains(item.id))
                    {
                        if (ransuu[{0, 3}] < 3) 
                        {
                            /* @todo merge gems more effectively */
                            int gems = ransuu[{1, farmables[item.id]}];
                            for (int i : {10, 5, 1}/* gem type */)
                                for (; gems >= i; gems -= i/* downgrade type */)
                                    im.emplace_back(112, i);
                        }

                        if (!ransuu[{0, 3}]) im.emplace_back(remember_id + 1, 1); // @note seed
                    }
                    else // @note normal drop rate
                    {
                        if (!ransuu[{0, 7}]) im.emplace_back(112, 1); // @todo get real growtopia gem drop amount.
                    }
                } /* ~gem drop */

                if (!ransuu[{0, 11}]) im.emplace_back(remember_id, 1); // @note block
                if (!ransuu[{0, 8}]) im.emplace_back(remember_id + 1, 1); // @note seed

skip_reset_tile: // @todo remove lazy method

                for (std::pair<short, short> &i : im)
                    item_change_object(event, {i.first, i.second},
                        {
                            static_cast<float>(state.punch[0]) + ransuu.shosu({7, 50}, 0.01f), // @note (0.07 - 0.50)
                            static_cast<float>(state.punch[1]) + ransuu.shosu({7, 50}, 0.01f)  // @note (0.07 - 0.50)
                        });
                        
                peer->add_xp(std::trunc(1.0f + items[remember_id].rarity / 5.0f));
                if (_bypass) return; // @todo remove lazy method
            }
        } // @note delete im, id
        else if (item.cloth_type != clothing::none) 
        {
            if (state.punch != std::array<int, 2zu>{ std::lround(peer->pos[0]), std::lround(peer->pos[1]) }) return;

            item_activate(event, state);
            return; 
        }
        else if (item.type == type::CONSUMEABLE) 
        {
            if (item.raw_name.contains(" Blast"))
            {
                packet::create(*event.peer, false, 0, {
                    "OnDialogRequest",
                    std::format(
                        "set_default_color|`o\n"
                        "embed_data|id|{0}\n"
                        "add_label_with_icon|big|`w{1}``|left|{0}|\n"
                        "add_label|small|This item creates a new world! Enter a unique name for it.|left\n"
                        "add_text_input|name|New World Name||24|\n"
                        "end_dialog|create_blast|Cancel|Create!|\n", // @todo rgt "Create!" is a purple-ish pink color
                        item.id, item.raw_name
                    ).c_str()
                });
            }
            if (item.raw_name.contains("Paint Bucket - "))
            {
                if (peer->clothing[hand] != 3494) throw std::runtime_error("you need a Paintbrush to apply paint!");
            }
            float effect{ 0.0f }; // @note allocate 'effect' only if peer is wearing a paint brush.
            switch (item.id)
            {
                case 1404: // @note Door Mover
                {
                    if (!door_mover(w->second, state.punch)) throw std::runtime_error("There's no room to put the door there! You need 2 empty spaces vertically.");

                    std::string remember_name = w->first;
                    action::quit_to_exit(event, "", true); // @todo everyone in world exits
                    action::join_request(event, "", remember_name); // @todo everyone in world re-joins
                    
                    break;
                }
                case 822: // @note Water Bucket
                {
                    block.state4 ^= S_WATER;
                    tile_update(event, std::move(state), block, w->second);
                    break;
                }
                case 1866: // @note Block Glue
                {
                    block.state4 ^= S_GLUE;
                    tile_update(event, std::move(state), block, w->second);
                    break;
                }
                case 3062: // @note Pocket Lighter
                {
                    block.state4 ^= S_FIRE; // @todo
                    tile_update(event, std::move(state), block, w->second);
                    break;
                }
                case 408: // @note Duct Tape
                {
                    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
                    {
                        auto &peers = _peer[&p];

                        if (state.punch == std::array<int, 2zu>{ std::lround(peers->pos[0]), std::lround(peers->pos[1]) }) // @todo improve accuracy
                        {
                            peers->state ^= S_DUCT_TAPE; // @todo add a 10 minute timer that will remove it.
                            ENetEvent event_perspective{.peer = &p};
                            on::SetClothing(event_perspective);
                        }
                    });
                    break;
                }
                case 3478: // @note Paint Bucket - Red
                {
                    block.state4 |= S_RED;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0x0000ff00; 
                    break;
                }
                case 3480: // @note Paint Bucket - Yellow
                {
                    block.state4 |= S_YELLOW;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0x00ffff00; // @note red + green
                    break;
                }
                case 3482: // @note Paint Bucket - Green
                {
                    block.state4 |= S_GREEN;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0x00ff0000;
                    break;
                }
                case 3484: // @note Paint Bucket - Aqua
                {
                    block.state4 |= S_AQUA;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0xffff0000; // @note blue + green
                    break;
                }
                case 3486: // @note Paint Bucket - Blue
                {
                    block.state4 |= S_BLUE;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0xff000000;
                    break;
                }
                case 3488: // @note Paint Bucket - Purple
                {
                    block.state4 |= S_PURPLE;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0xff00ff00; // @note blue + red
                    break;
                }
                case 3490: // @note Paint Bucket - Charcoal
                {
                    block.state4 |= S_CHARCOAL;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0xffffffff; // @note B(blue)G(green)R(red)A(alpha/opacity) max will provide a pure black color. idk if growtopia is the same.
                    break;
                }
                case 3492: // @note Paint Bucket - Vanish
                {
                    block.state4 &= ~S_VANISH;
                    tile_update(event, std::move(state), block, w->second);
                    effect = 0xffffff00; // @todo get exact color. I just guessed T-T
                }
            }
            if (effect > 0.0f)
            {
                state_visuals(event, ::state{
                    .type = 0x11,
                    .pos = { static_cast<float>((state.punch.front() * 32) + 16), static_cast<float>((state.punch.back() * 32) + 16) },
                    .speed = { effect, 0xa8 }
                });
            }

            if (item.id == 6336) return; // @todo
            peer->emplace(slot(item.id, -1));
            modify_item_inventory(event, {(short)item.id, 1});
            return;
        }
        else if (state.id == 32)
        {
            switch (item.type)
            {
                case type::LOCK: // @todo handle sl, bl, hl, builder lock, ect.
                {
                    if (peer->user_id == w->second.owner)
                    {
                        packet::create(*event.peer, false, 0, {
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
                                "add_checkbox|checkbox_public|Allow anyone to Build and Break|{}\n"
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
                                item.raw_name, item.id, state.punch[0], state.punch[1], to_char(w->second._public)
                            ).c_str()
                        });
                    }
                    break;
                }
                case type::DOOR:
                case type::PORTAL:
                {
                    std::string dest, id{};
                    for (::door& door : w->second.doors)
                        if (door.pos == state.punch) dest = door.dest, id = door.id;
                        
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_text_input|door_name|Label|{}|100|\n"
                            "add_popup_name|DoorEdit|\n"
                            "add_text_input|door_target|Destination|{}|24|\n"
                            "add_smalltext|Enter a Destination in this format: `2WORLDNAME:ID``|left|\n"
                            "add_smalltext|Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.|left|\n"
                            "add_text_input|door_id|ID|{}|11|\n"
                            "add_smalltext|Set a unique `2ID`` to target this door as a Destination from another!|left|\n"
                            "add_checkbox|checkbox_locked|Is open to public|1\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|door_edit|Cancel|OK|", 
                            item.raw_name, item.id, block.label, dest, id, state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
                }
                case type::SIGN:
                        packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "add_popup_name|SignEdit|\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_textbox|What would you like to write on this sign?``|left|\n"
                            "add_text_input|sign_text||{}|128|\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|sign_edit|Cancel|OK|", 
                            item.raw_name, item.id, block.label, state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
                case type::ENTRANCE:
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format(
                            "set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_checkbox|checkbox_public|Is open to public|{}\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|gateway_edit|Cancel|OK|\n", 
                            item.raw_name, item.id, to_char(block._public), state.punch[0], state.punch[1]
                        ).c_str()
                    });
                    break;
            }
            return; // @note leave early else wrench will act as a block unlike fist which breaks. this is cause of state_visuals()
        }
        else // @note placing a block
        {
            switch (item.type)
            {
                case type::LOCK:
                {
                    if (!w->second.owner)
                    {
                        w->second.owner = peer->user_id;
                        if (!peer->role) peer->prefix.front() = '2';
                        state.type = 0x0f;
                        state.netid = w->second.owner;
                        state.peer_state = 0x08;
                        state.id = state.id;
                        if (std::ranges::find(peer->my_worlds, w->second.name) == peer->my_worlds.end()) 
                        {
                            std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
                            peer->my_worlds.back() = w->second.name;
                        }
                        peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
                        {
                            std::string placed_message{ std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", w->first, peer->ltoken[0]) };
                            packet::create(p, false, 0, {
                                "OnTalkBubble", 
                                peer->netid,
                                placed_message.c_str(),
                                0u
                            });
                            packet::create(p, false, 0, {
                                "OnConsoleMessage",
                                placed_message.c_str()
                            });
                        });
                        on::NameChanged(event);
                    }
                    else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
                    break;
                }
                case type::PROVIDER:
                {
                    block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
                    block.tick = steady_clock::now();
                    break;
                }
                case type::SEED:
                {
                    if (block.fg != 0) return; // @todo add splicing
                    block.tick = steady_clock::now();
                    break;
                }
                case type::WEATHER_MACHINE:
                {
                    block.toggled = true;
                    peers(event, PEER_SAME_WORLD, [state](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", get_weather_id(state.id) });
                    });
                    for (::block &b : w->second.blocks)
                        if (items[b.fg]/*@todo*/.type == type::WEATHER_MACHINE && b.fg != state.id) b.toggled = false;
                    break;
                }
            }
            if (item.collision == collision::full)
            {
                // 이 (left, right)
                bool x = state.punch.front() == std::lround(state.pos.front() / 32);
                // 으 (up, down)
                bool y = state.punch.back() == std::lround(state.pos.back() / 32);

                if ((x && y)) return; // @todo when moving avoid collision.
            }
            (item.type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
            peer->emplace(slot(state.id, -1));
            modify_item_inventory(event, {(short)item.id, 1});
        }
        if (state.netid != w->second.owner) state.netid = peer->netid;
        state_visuals(event, std::move(state)); // finished.
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) 
            packet::create(*event.peer, false, 0, {
                "OnTalkBubble", 
                _peer[event.peer]->netid, 
                exc.what(),
                0u,
                1u // @note message will be sent once instead of multiple times.
            });
    }
}
