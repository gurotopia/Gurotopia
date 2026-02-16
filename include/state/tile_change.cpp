#include "pch.hpp"
#include "on/NameChanged.hpp"
#include "on/SetClothing.hpp"
#include "commands/weather.hpp"
#include "item_activate.hpp"
#include "tools/ransuu.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "action/quit_to_exit.hpp"
#include "action/join_request.hpp"
#include "on/Action.hpp"
#include "item_activate_object.hpp"
#include "tile_change.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

void tile_change(ENetEvent& event, state state) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    try
    {
        if (!worlds.contains(peer->recent_worlds.back())) return;
        ::world &world = worlds.at(peer->recent_worlds.back());

        ::block &block = world.blocks[cord(state.punch.x, state.punch.y)];

        auto item = std::ranges::find(items, (state.id != 32 && state.id != 18) ? state.id : (block.fg != 0) ? block.fg : block.bg, &::item::id);
        if (item->id == 0) return;

        if (block.state4 & S_FIRE) // @note allow anyone to take out fire
            if (peer->clothing[hand] == 3066/* fire hose */)
            {
                remove_fire(event, state, block, world);
                return; // @note avoid hitting the block
            }

        if (!(item->cat & CAT_PUBLIC)) // @note if block is public skip validating if peer is owner or admin
            if ((world.owner && !world.is_public && !peer->role) &&
                (peer->user_id != world.owner && !std::ranges::contains(world.admin, peer->user_id))) return;

        bool lock_visuals{}; // @todo this looks sloppy
        
        if (state.id == 18) // @note punching a block
        {
            ransuu ransuu;

            switch (item->id)
            {
                case 758: // @note Roulette Wheel
                {
                    u_char number = ransuu[{0, 36}];
                    char color = (number == 0) ? '2' : (ransuu[{0, 3}] < 2) ? 'b' : '4';
                    std::string message = std::format("[`{}{}`` spun the wheel and got `{}{}``!]", peer->prefix, peer->ltoken[0], color, number);
                    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, message](ENetPeer& p)
                    {
                        packet::create(p, false, 2000, { "OnTalkBubble", peer->netid, message.c_str() });
                        packet::create(p, false, 2000, { "OnConsoleMessage", message.c_str() });
                    });
                    break;
                }
            }
            switch (item->type)
            {
                case type::STRONG: throw std::runtime_error("It's too strong to break.");
                case type::MAIN_DOOR: throw std::runtime_error("(stand over and punch to use)");
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @todo seperate area for 'range_lock'
                    
                    if (world.owner != peer->user_id)
                        throw std::runtime_error(std::format("`5[```w{}`` `$World Locked`` by (null)`5]``", world.name)); // @todo add owner name
                }
                case type::PROVIDER:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick) // @todo limit this check.
                    {
                        switch (item->id)
                        {
                            case 1008: // @note ATM
                            {
                                u_char gems = ransuu[{1, 100}]; // @note source: https://growtopia.fandom.com/wiki/ATM_Machine
                                for (short i : {100, 50, 10, 5, 1}/* gem type */)
                                    for (; gems >= i; gems -= i/* downgrade type */)
                                        add_drop(event, {112, i}, state.punch);
                                        
                                break;
                            }
                            case 872:/*chicken*/ case 866:/*cow*/ case 1632:/*coffee maker*/ case 3888:/*sheep*/
                            {
                                add_drop(event, ::slot(item->id+2, ransuu[{1, 2}]), state.punch);
                                break;
                            }
                            case 5116:/*tea set*/
                            {
                                add_drop(event, ::slot(item->id-2, ransuu[{1, 2}]), state.punch);
                                break;
                            }
                            case 2798:/*well*/
                            {
                                add_drop(event, ::slot(822/*water bucket*/, ransuu[{1, 2}]), state.punch);
                                break;
                            }
                            case 928:/*science station*/ // @note source: https://growtopia.fandom.com/wiki/Science_Station
                            {
                                short chemcial = 
                                    (!ransuu[{0, 16}]) ? chemcial = 918/*P*/ : 
                                    (!ransuu[{0, 8}])  ? chemcial = 920/*B*/ : 
                                    (!ransuu[{0, 6}])  ? chemcial = 924/*Y*/ : 
                                    (!ransuu[{0, 4}])  ? chemcial = 916/*R*/ : chemcial = 914/*G*/;
                                add_drop(event, {chemcial, 1}, state.punch);
                                break;
                            }
                        }
                        block.tick = steady_clock::now();
                        send_tile_update(event, std::move(state), block, world); // @note update countdown on provider.

                        peer->add_xp(event, 1);
                        return;
                    }
                    break;
                }
                case type::SEED:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick) // @todo limit this check.
                    {
                        block.hits[0] = 99;
                        add_drop(event, ::slot(item->id - 1, ransuu[{2, 12}]), state.punch); // @note fruit (from tree)
                    }
                    break;
                }
                case type::WEATHER_MACHINE:
                {
                    for (::block &b : world.blocks)
                    {
                        auto item = std::ranges::find(items, b.fg, &::item::id);
                        if (item->type == type::WEATHER_MACHINE && item->id != block.fg) 
                            b.state3 &= ~S_TOGGLE;
                    }
                    block.state3 ^= S_TOGGLE;
                    
                    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [block, item](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", (block.state3 & S_TOGGLE) ? get_weather_id(item->id) : 0 });
                    });
                    break;
                }
                case type::TOGGLEABLE_BLOCK:
                case type::TOGGLEABLE_ANIMATED_BLOCK:
                case type::CHEST:
                {
                    block.state3 ^= S_TOGGLE;
                    if (item->id == 226) // @note Signal Jammer
                    {
                        packet::create(*event.peer, false, 0, {
                            "OnConsoleMessage",
                            (block.state3 & S_TOGGLE) ? 
                                "Signal jammer enabled. This world is now `4hidden`` from the universe." :
                                "Signal jammer disabled.  This world is `2visible`` to the universe."
                        });
                    }
                    break;
                }
            }
            tile_apply_damage(event, std::move(state), block);

            if (block.hits.front() >= item->hits) block.fg = 0, block.hits.front() = 0;
            else if (block.hits.back() >= item->hits) block.bg = 0, block.hits.back() = 0;
            else return;
            
            /* @todo update these changes with tile_update() */
            block.label = "";
            block.state3 = 0x00; // @note reset tile direction
            block.state4 &= ~S_VANISH; // @note remove paint
            
            if (item->id == 392/*Heartstone*/ || item->id == 3402/*GBC*/ || item->id == 9350/*Super GBC*/)
            {
                short reward =
                    (!ransuu[{0, 99}]) ? 1458 : // @note GHC
                    (!ransuu[{0, 20}]) ? 362 : // @note Angel Wings
                    (!ransuu[{0, 8}])  ? 366 : // @note Heartbow
                    (!ransuu[{0, 8}])  ? 1470 : // @note Ruby Necklace
                    (!ransuu[{0, 20}]) ? 2384 : // @note Love Bug
                    (!ransuu[{0, 4}])  ? 2396 : // @note Valensign
                    (!ransuu[{0, 10}]) ? 3388 : // @note Heartbreaker Hammer
                    (!ransuu[{0, 10}]) ? 2390 : // @note Teeny Angel Wings
                    (!ransuu[{0, 10}]) ? 3396 : // @note Lovebird Pendant
                    (!ransuu[{0, 2}])  ? 3404 : // @note Sour Lollipop
                    (!ransuu[{0, 4}])  ? 3406 : // @note Sweet Lollipop
                    (!ransuu[{0, 2}])  ? 3408 : // @note Pink Marble Arch
                    388; // @note Perfume
                    // @todo add all the remaining drops - https://growtopia.fandom.com/wiki/Golden_Booty_Chest

                add_drop(event, ::slot(reward, (reward == 3408 || reward == 3404) ? 10 : 1), state.punch);
                if (reward == 1458)
                {
                    std::string message = std::format("msg|`4The Power of Love! `2{} found a `#Golden Heart Crystal`2 in a `#{}`2!", peer->ltoken[0], item->raw_name);
                    peers(peer->recent_worlds.back(), PEER_ALL, [message](ENetPeer &p)
                    {
                        packet::action(p, "log", message.c_str());
                    });
                }
                if (++peer->gbc_pity % 100 == 0) modify_item_inventory(event, ::slot{9350, 1});
            }
            else if (item->type == type::LOCK && !is_tile_lock(item->id))
            {
                if (!peer->role)
                {
                    peer->prefix.front() = 'w';
                    on::NameChanged(event);
                }
                
                world.owner = 0; // @todo have a seperate thing for 'range_lock'
            }

            if (item->cat == CAT_RETURN)
            {
                int uid = item_change_object(event, ::slot(item->id, 1), state.pos);
                item_activate_object(event, ::state{.id = uid, .punch = state.punch});
            }
            else if (u_char(item->property) & 04) { } // @note "This item never drops any seeds."; should it drop a block?
            else // @note normal break (drop gem, seed, block & give XP)
            {
                if (item->type != type::SEED)
                { /* gem drop */
                    /* if greater than 1, assume it's a farmable.*/
                    u_char rarity_to_gem =
                        (item->rarity >= 87) ? 22 : 
                        (item->rarity >= 68) ? 18 : 
                        (item->rarity >= 53) ? 14 : 
                        (item->rarity >= 41) ? 11 : 
                        (item->rarity >= 36) ? 10 :
                        (item->rarity >= 32) ? 9 :
                        (item->rarity >= 24) ? 5 : 1;

                    if (!ransuu[{0, (rarity_to_gem > 1) ? 1 : 4}]) // @note double chances if farmable.
                    {
                        /* @todo merge gems more effectively */
                        u_char gems = ransuu[{1, rarity_to_gem}];
                        for (short i : {10, 5, 1}/* gem type */)
                            for (; gems >= i; gems -= i/* downgrade type */)
                                add_drop(event, {112, i}, state.punch);
                    }
                    if (!ransuu[{0, (rarity_to_gem > 1) ? 2 : 4}]) add_drop(event, ::slot(item->id + 1, 1), state.punch);
                    else if (!ransuu[{0, (rarity_to_gem > 1) ? 4 : 8}]) add_drop(event, ::slot(item->id, 1), state.punch);
                } /* ~gem drop */

                peer->add_xp(event, std::trunc(1.0f + item->rarity / 5.0f));
            }
        } // @note delete im, id
        else if (item->cloth_type != clothing::none) 
        {
            if (state.punch != peer->pos) return;

            item_activate(event, state);
            return; 
        }
        else if (item->type == type::CONSUMEABLE) 
        {
            if (item->raw_name.contains(" Blast"))
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
                        item->id, item->raw_name
                    ).c_str()
                });
            }

            if (item->raw_name.contains("Paint Bucket - ") && peer->clothing[hand] != 3494) throw std::runtime_error("you need a Paintbrush to apply paint!");
            float color{}; // @note the color of the paint particle effect.
            float particle{};
            switch (item->id)
            {
                case 1404: // @note Door Mover
                {
                    if (!door_mover(world, state.punch)) throw std::runtime_error("There's no room to put the door there! You need 2 empty spaces vertically.");

                    std::string remember_name = world.name;
                    action::quit_to_exit(event, "", true); // @todo everyone in world exits
                    action::join_request(event, "", remember_name); // @todo everyone in world re-joins
                    
                    break;
                }
                case 822: // @note Water Bucket
                {
                    if (block.state4 & S_FIRE) remove_fire(event, state, block, world);
                    else block.state4 ^= S_WATER;
                    break;
                }
                case 1866: // @note Block Glue
                {
                    block.state4 ^= S_GLUE;
                    break;
                }
                case 3062: // @note Pocket Lighter
                {
                    if (block.fg == 0 && block.bg == 0) throw std::runtime_error("There's nothing to burn!");
                    if (block.state4 & (S_FIRE | S_WATER)) return; // @note avoid fire on water & fire on fire

                    block.state4 |= S_FIRE;

                    std::string message = "`7[```4MWAHAHAHA!! FIRE FIRE FIRE```7]``";
                    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
                    {
                        packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, message.c_str(), 0u });
                        packet::create(*event.peer, false, 0, { "OnConsoleMessage", message.c_str() });
                    });
                    particle = 0x96;

                    if (block.fg == 3090) // @note Highly Combustible Box
                    {
                        block.fg = 3128; // @note Combusted Box
                        if (!(block.state3 & S_TOGGLE)/*closed*/) {} // @todo recipes: https://growtopia.fandom.com/wiki/Guide:Highly_Combustible_Box
                    }
                    break;
                }
                case 3404:/*Sour Lollipop*/ case 3406:/*Sweet Lollipop*/
                {
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "`#YUM!:D", 0u });

                    break;
                }
                case 3400: // @note Love Potion #8
                {
                    if (block.fg != 10) return; // @note Rock

                    block.fg = 392; // @note Heartstone
                    particle = 0x2c;
                    break;
                }
                case 1488: // @note Experience Potion
                {
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "`#GULP! You got smarter!", 0u });
                    peer->add_xp(event, 10000);
                    break;
                }
                case 2480: // @note Megaphone
                {
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .set_default_color("`o")
                            .add_label_with_icon("big", "`wMegaphone``", item->id)
                            .add_textbox("Enter a message you want to broadcast to every player in Growtopia! This will use up 1 Megaphone")
                            .add_text_input("message", "", "", 128)
                            .end_dialog("megaphone", "Nevermind", "Broadcast").c_str()
                    });
                    break;
                }
                case 408: // @note Duct Tape
                {
                    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
                    {
                        ::peer *_p = static_cast<::peer*>(p.data);

                        if (state.punch == peer->pos) // @todo improve accuracy
                        {
                            _p->state ^= S_DUCT_TAPE; // @todo add a 10 minute timer that will remove it.
                            on::SetClothing(p);
                        }
                    });
                    break;
                }
                case 3478: // @note Paint Bucket - Red
                {
                    block.state4 |= S_RED;
                    color = 0x0000ff00, particle = 0xa8; 
                    break;
                }
                case 3480: // @note Paint Bucket - Yellow
                {
                    block.state4 |= S_YELLOW;
                    color = 0x00ffff00, particle = 0xa8; // @note red + green
                    break;
                }
                case 3482: // @note Paint Bucket - Green
                {
                    block.state4 |= S_GREEN;
                    color = 0x00ff0000, particle = 0xa8;
                    break;
                }
                case 3484: // @note Paint Bucket - Aqua
                {
                    block.state4 |= S_AQUA;
                    color = 0xffff0000, particle = 0xa8; // @note blue + green
                    break;
                }
                case 3486: // @note Paint Bucket - Blue
                {
                    block.state4 |= S_BLUE;
                    color = 0xff000000, particle = 0xa8;
                    break;
                }
                case 3488: // @note Paint Bucket - Purple
                {
                    block.state4 |= S_PURPLE;
                    color = 0xff00ff00, particle = 0xa8; // @note blue + red
                    break;
                }
                case 3490: // @note Paint Bucket - Charcoal
                {
                    block.state4 |= S_CHARCOAL;
                    color = 0xffffffff, particle = 0xa8; // @note B(blue)G(green)R(red)A(alpha/opacity) max will provide a pure black color. idk if growtopia is the same.
                    break;
                }
                case 3492: // @note Paint Bucket - Vanish
                {
                    block.state4 &= ~S_VANISH;
                    color = 0xffffff00, particle = 0xa8; // @todo get exact color. I just guessed T-T
                }
                default: return; // @note prevent taking the consumeable if nothing happended
            }
            if (particle > 0.0f)
            {
                state_visuals(*event.peer, ::state{
                    .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
                    .pos = state.punch,
                    .speed = { color, particle }
                });
            }
            send_tile_update(event, std::move(state), block, world);

            modify_item_inventory(event, ::slot(item->id, -1));
            peer->add_xp(event, 1);
            return;
        }
        else if (state.id == 32)
        {
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @todo seperate area for 'range_lock'

                    if (peer->user_id == world.owner)
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
                                "add_checkbox|checkbox_disable_music|Disable Custom Music Blocks|{}\n"
                                "add_text_input|tempo|Music BPM|100|3|\n"
                                "add_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|0\n"
                                //"add_smalltext|Your current home world is: JOLEIT|left|\n"           // @todo only show when peer has a set home world.
                                "add_checkbox|checkbox_set_as_home_world|Set as Home World|0|\n"
                                "add_text_input|minimum_entry_level|World Level: |{}|3|\n"
                                "add_smalltext|Set minimum world entry level.|\n"
                                "add_button|sessionlength_dialog|`wSet World Timer``|noflags|0|0|\n"
                                "add_button|changecat|`wCategory: None``|noflags|0|0|\n"
                                "add_button|getKey|Get World Key|noflags|0|0|\n"
                                "end_dialog|lock_edit|Cancel|OK|\n",
                                item->raw_name, item->id, state.punch.x, state.punch.y, to_char(world.is_public), (world.lock_state & DISABLE_MUSIC) ? "1" : "0", world.minimum_entry_level
                            ).c_str()
                        });
                    }
                    break;
                }
                case type::DOOR:
                case type::PORTAL:
                {
                    std::string dest, id{};
                    for (::door& door : world.doors)
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
                            item->raw_name, item->id, block.label, dest, id, state.punch.x, state.punch.y
                        ).c_str()
                    });
                    break;
                }
                case type::SIGN:
                {
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
                            item->raw_name, item->id, block.label, state.punch.x, state.punch.y
                        ).c_str()
                    });
                    break;
                }
                case type::ENTRANCE:
                {
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        std::format(
                            "set_default_color|`o\n"
                            "add_label_with_icon|big|`wEdit {}``|left|{}|\n"
                            "add_checkbox|checkbox_public|Is open to public|{}\n"
                            "embed_data|tilex|{}\n"
                            "embed_data|tiley|{}\n"
                            "end_dialog|gateway_edit|Cancel|OK|\n", 
                            item->raw_name, item->id, to_char((block.state3 & S_PUBLIC)), state.punch.x, state.punch.y
                        ).c_str()
                    });
                    break;
                }
                case type::DISPLAY_BLOCK:
                {
                    packet::create(*event.peer, false, 0, {
                        "OnDialogRequest",
                        ::create_dialog()
                            .set_default_color("`o")
                            .add_label_with_icon("big", "Display Block", item->id) // @todo
                    });
                    break;
                }
            }
            return; // @note leave early else wrench will act as a block unlike fist which breaks. this is cause of state_visuals()
        }
        else // @note placing a block
        {
            if (item->type != type::SEED && (item->type != type::BACKGROUND && block.fg != 0))
            {
                bool update_tile{};
                switch (items[world.blocks[cord(state.punch.x, state.punch.y)].fg].type)
                {
                    case type::DISPLAY_BLOCK:
                    {
                        world.displays.emplace_back(::display(item->id, state.punch));
                        update_tile = true;
                        break;
                    }
                }
                if (update_tile)
                    send_tile_update(event, std::move(state), block, world);
                return;
            }
            if (item->collision == collision::FULL)
            {
                if (state.punch.x == state.pos.x && state.punch.y == state.pos.y) return; // @todo when moving avoid collision.
            }
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @note seperate area for 'range_lock'

                    if (!world.owner)
                    {
                        world.owner = peer->user_id;
                        lock_visuals = true;
                        if (!peer->role) 
                        {
                            peer->prefix.front() = '2';
                            on::NameChanged(event);
                        }
                        if (std::ranges::find(peer->my_worlds, world.name) == peer->my_worlds.end()) 
                        {
                            std::ranges::rotate(peer->my_worlds, peer->my_worlds.begin() + 1);
                            peer->my_worlds.back() = world.name;
                        }
                        std::string placed_message = std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world.name, peer->ltoken[0]);
                        peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&peer, placed_message](ENetPeer& p) 
                        {
                            packet::create(p, false, 0, { "OnTalkBubble", peer->netid, placed_message.c_str() });
                            packet::create(p, false, 0, { "OnConsoleMessage", placed_message.c_str() });
                        });
                    }
                    else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
                    break;
                }
                case type::ENTRANCE:
                {
                    block.state3 |= S_PUBLIC;
                    break;
                }
                case type::PROVIDER:
                {
                    block.tick = steady_clock::now();
                    break;
                }
                case type::SEED:
                {
                    block.state3 |= 0x11;
                    /* forgive the messy code. this was rushed. ~leeendl */
                    bool spliced{};
                    for (::item &item : items)
                    {
                        if ((item.splice[0] == state.id && item.splice[1] == block.fg) ||
                            (item.splice[1] == state.id && item.splice[0] == block.fg) /* allow reverse splice combo */)
                        {
                            auto splice0 = std::ranges::find(items, item.splice[0], &::item::id);
                            auto splice1 = std::ranges::find(items, item.splice[1], &::item::id);

                            state.id = item.id;
                            packet::create(*event.peer, false, 0, {
                                "OnTalkBubble", 
                                peer->netid, 
                                std::format("`w{}`` and `w{}`` have been spliced to make a `${} Tree``!", 
                                    splice0->raw_name, splice1->raw_name, item.raw_name.substr(0, item.raw_name.length()-5/* seed*/)).c_str(), // @todo this is hardcoded
                                0u,
                                1u
                            });
                            spliced = true;
                            break;
                        }
                    }
                    if (block.fg != 0 && !spliced) return;

                    block.tick = steady_clock::now();

                    /* @todo change this */
                    block.fg = state.id;
                    send_tile_update(event, std::move(state), block, world);

                    break;
                }
            }
            block.state3 |= (peer->facing_left) ? S_LEFT : S_RIGHT;
            (item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
            peer->emplace(::slot(item->id, -1));
        }
        state.netid = peer->netid; // @todo sometimes rgt has this as 0
        state_visuals(*event.peer, std::move(state)); // finished.
        if (lock_visuals) 
        {
            state_visuals(*event.peer, ::state{
                .type = 0x0f, // @note PACKET_SEND_LOCK
                .netid = world.owner, 
                .peer_state = 0x08, 
                .id = state.id,
                .punch = state.punch
            });
        }
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) 
            packet::create(*event.peer, false, 0, {
                "OnTalkBubble", 
                peer->netid, 
                exc.what(),
                0u,
                1u // @note message will be sent once instead of multiple times.
            });
        return;
    }
}
