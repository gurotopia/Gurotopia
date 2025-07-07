#include "pch.hpp"
#include "on/NameChanged.hpp"
#include "commands/weather.hpp"
#include "equip.hpp"
#include "tile_change.hpp"

#include "tools/ransuu.hpp"

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
        if (!create_rt(event, 0, 160)) return;
        auto &peer = _peer[event.peer];
        auto w = worlds.find(peer->recent_worlds.back());
        if (w == worlds.end()) return;

        if ((w->second.owner != 0 && !w->second._public && peer->role == role::PLAYER) &&
            (peer->user_id != w->second.owner && !std::ranges::contains(w->second.admin, peer->user_id))) return;

        block &block = w->second.blocks[cord(state.punch[0], state.punch[1])];
        item &item = (state.id != 32 && state.id != 18) ? items[state.id] : (block.fg != 0) ? items[block.fg] : items[block.bg];
        if (state.id == 18) // @note punching a block
        {
            if (item.id == 0) return;
            if (item.type == std::byte{ type::STRONG }) throw std::runtime_error("It's too strong to break.");
            if (item.type == std::byte{ type::MAIN_DOOR }) throw std::runtime_error("(stand over and punch to use)");

            std::vector<std::pair<short, short>> im{}; // @note list of dropped items
            ransuu ransuu;
            switch (item.type)
            {
                case std::byte{ type::LOCK }: // @todo add message saying who owns the lock.
                {
                    if (peer->user_id != w->second.owner) return;
                    break;
                }
                case std::byte{ type::SEED }:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item.tick)
                    {
                        block.hits[0] = 999;
                        im.emplace_back(item.id - 1, ransuu[{0, 8}]); // @note fruit (from tree)
                    }
                    break;
                }
                case std::byte{ type::WEATHER_MACHINE }:
                {
                    int remember_weather{0};
                    if (!block.toggled) 
                    {
                        block.toggled = true;
                        remember_weather = get_weather_id(item.id);
                    }
                    else block.toggled = false;
                    peers(event, PEER_SAME_WORLD, [remember_weather](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", remember_weather });
                    });
                    for (::block &b : w->second.blocks)
                        if (items[b.fg]/*@todo*/.type == std::byte{ type::WEATHER_MACHINE } && b.fg != block.fg) b.toggled = false;
                    
                    break;
                }
                case std::byte{ type::TOGGLEABLE_BLOCK }:
                case std::byte{ type::TOGGLEABLE_ANIMATED_BLOCK }:
                {
                    if (!block.toggled) 
                    {
                        block.toggled = true;
                        if (item.id == 226)
                        {
                            packet::create(*event.peer, false, 0, {
                                "OnConsoleMessage",
                                "Signal jammer enabled. This world is now `4hidden`` from the universe."
                            });
                        }
                    } 
                    else block.toggled = false;
                    break;
                }
            }
            block_punched(event, std::move(state), block);
            
            short remember_id = item.id;
            if (block.hits.front() >= item.hits) block.fg = 0, block.hits.front() = 0;
            else if (block.hits.back() >= item.hits) block.bg = 0, block.hits.back() = 0;
            else return;
            block.label = ""; // @todo
            block.toggled = false; // @todo

            if (item.cat == std::byte{ 02 }) // pick up (item goes back in your inventory)
            {
                drop_visuals(event, {remember_id, 1}, {state.pos[0] / 32, state.pos[1] / 32}); // @todo
                inventory_visuals(event);
            }
            else // normal break (drop gem, seed, block & give XP)
            {

                if (ransuu[{0, 9}] <= 1) im.emplace_back(112, 1); // @todo get real growtopia gem drop amount.
                if (item.type != std::byte{ type::SEED })
                {
                    if (ransuu[{0, 17}] <= 1) im.emplace_back(remember_id, 1);
                    if (ransuu[{0, 11}] <= 1) im.emplace_back(remember_id + 1, 1);
                }
                for (std::pair<short, short> &i : im)
                    drop_visuals(event, {i.first, i.second},
                        {
                            static_cast<float>(state.punch[0]) + ransuu.shosu({7, 50}, 0.01f), // @note (0.07 - 0.50)
                            static_cast<float>(state.punch[1]) + ransuu.shosu({7, 50}, 0.01f)  // @note (0.07 - 0.50)
                        });
                        
                peer->add_xp(std::trunc(1.0f + items[remember_id].rarity / 5.0f));
            }
        } // @note delete im, id
        else if (item.cloth_type != clothing::none) 
        {
            equip(event, state); // @note imitate equip
            return; 
        }
        else if (item.type == std::byte{ type::CONSUMEABLE }) return;
        else if (state.id == 32)
        {
            switch (item.type)
            {
                case std::byte{ type::LOCK }: // @todo handle sl, bl, hl, builder lock, ect.
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
                                item.raw_name, item.id, state.punch[0], state.punch[1], signed{w->second._public}
                            ).c_str()
                        });
                    }
                    break;
                }
                case std::byte{ type::DOOR }:
                case std::byte{ type::PORTAL }:
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
                case std::byte{ type::SIGN }:
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
                case std::byte{ type::ENTRANCE }:
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format("set_default_color|`o\n"
                            "set_default_color|`o"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|"
                            "add_checkbox|checkbox_public|Is open to public|1"
                            "embed_data|tilex|{}"
                            "embed_data|tiley|{}"
                            "end_dialog|gateway_edit|Cancel|OK|", 
                            item.raw_name, item.id, state.punch[0], state.punch[1]
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
                case std::byte{ type::LOCK }:
                {
                    if (w->second.owner == 00)
                    {
                        w->second.owner = peer->user_id;
                        if (peer->role == role::PLAYER) peer->prefix = "2";
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
                case std::byte{ type::SEED }:
                case std::byte{ type::PROVIDER }:
                {
                    if (block.fg != 0) return; // @todo add splicing
                    block.tick = steady_clock::now();
                    break;
                }
                case std::byte{ type::WEATHER_MACHINE }:
                {
                    block.toggled = true;
                    peers(event, PEER_SAME_WORLD, [state](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", get_weather_id(state.id) });
                    });
                    for (::block &b : w->second.blocks)
                        if (items[b.fg]/*@todo*/.type == std::byte{ type::WEATHER_MACHINE } && b.fg != state.id) b.toggled = false;
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
            (item.type == std::byte{ type::BACKGROUND }) ? block.bg = state.id : block.fg = state.id;
            peer->emplace(slot(state.id, -1));
            inventory_visuals(event);
        }
        if (state.netid != w->second.owner) state.netid = peer->netid;
        state_visuals(event, std::move(state)); // finished.
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) 
            packet::create(*event.peer, false, 0, {
                "OnTalkBubble", 
                _peer[event.peer]->netid, // @note we are not using 'peer' ref cause of ratelimit and waste of memory.
                exc.what()
            });
    }
}