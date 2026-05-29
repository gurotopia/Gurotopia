#include "pch.hpp"
#include "on/NameChanged.hpp"
#include "on/SetClothing.hpp"
#include "on/Action.hpp"
#include "commands/weather.hpp"
#include "item_activate.hpp"
#include "tools/ransuu.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "action/quit_to_exit.hpp"
#include "action/join_request.hpp"
#include "action/dialog_return/vending.hpp"
#include "item_activate_object.hpp"
#include "tile_change.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

namespace {
    inline u_int tree_elapsed_seconds(const ::block& block) {
        return static_cast<u_int>((steady_clock::now() - block.tick) / 1s);
    }

    inline u_char tree_fruit_visual_count(const ::item& item, const ::block& block) {
        if (item.type != type::SEED) return 0;
        if (tree_elapsed_seconds(block) < static_cast<u_int>(std::max(0, item.tick))) return 0;
        return static_cast<u_char>((item.cat & CAT_SUPRISING_FRUIT) ? 4 : 3);
    }

    inline bool apply_tree_growth_spray(ENetEvent& event, ::peer& pPeer, ::world& world, ::state state, ::block& block, const ::item& tree_item, int fast_seconds, std::string_view success_text) {
        if (tree_item.type != type::SEED) throw std::runtime_error("You can only use that on a tree!");

        const int grow_time = std::max(0, tree_item.tick);
        const u_int elapsed = tree_elapsed_seconds(block);
        if (elapsed >= static_cast<u_int>(grow_time))
            throw std::runtime_error("That tree is already fully grown!");

        const int remaining = std::max(0, grow_time - static_cast<int>(elapsed));
        const int applied = std::min(fast_seconds, remaining);
        if (applied <= 0) return false;

        block.tick -= seconds(applied);
        send_tile_update(event, state, block, world);

        std::string text(success_text);
        packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer.netid, text.c_str(), 0u, 1u });
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", text.c_str() });
        return true;
    }
}

void tile_change(ENetEvent& event, state state) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    try
    {
        auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;

        ::block &block = world->blocks[cord(state.punch.x, state.punch.y)];

        auto item = std::ranges::find(items, (state.id != 32 && state.id != 18) ? state.id : (block.fg != 0) ? block.fg : block.bg, &::item::id);
        if (item->id == 0) return;

        if (block.state[3] & S_FIRE) // @note allow anyone to take out fire
            if (pPeer->clothing[hand] == 3066/* fire hose */)
            {
                remove_fire(event, state, block, *world);
                return; // @note avoid hitting the block
            }

        if (!(item->cat & CAT_PUBLIC)) // @note if block is public skip validating if peer is owner or admin
            if ((world->owner && !world->is_public && !pPeer->role) &&
                (pPeer->user_id != world->owner && !std::ranges::contains(world->admin, pPeer->user_id))) return;

        bool lock_visuals{}; // @todo this looks sloppy
        
        if (state.id == 18) // @note punching a block
        {
            static bool punch{}; // @note true if tile_change has been called within this inital (punch)

            if (!punch) // @note put all multiple punch features here
            {
                punch = true;
                if (pPeer->clothing[hand] == 5480) // @note Rayman's Fist
                {
                    ::state copy_state = state;

                    /* @note up and down */
                    if (state.punch.y == state.pos.by_32(true).y)
                    {
                        copy_state.punch.x += (pPeer->facing_left) ? -1 : 1;
                        tile_change(event, std::move(copy_state));
                        
                        copy_state.punch.x += (pPeer->facing_left) ? -1 : 1;
                        tile_change(event, std::move(copy_state));
                    }
                    /* @note left and right <- -> */
                    else if (state.punch.x == state.pos.by_32(true).x)
                    {
                        copy_state.punch.y += (state.punch.y < state.pos.by_32(true).y) ? -1 : 1;
                        tile_change(event, std::move(copy_state));

                        copy_state.punch.y += (state.punch.y < state.pos.by_32(true).y) ? -1 : 1;
                        tile_change(event, std::move(copy_state));
                    }
                    /* @note horizontal adjacent \/ */
                    else if (state.punch.y != state.pos.by_32(true).y)
                    {
                        copy_state.punch.x += (pPeer->facing_left) ? -1 : 1;
                        copy_state.punch.y += (state.punch.y < state.pos.by_32(true).y) ? -1 : 1;
                        tile_change(event, std::move(copy_state));

                        copy_state.punch.x += (pPeer->facing_left) ? -1 : 1;
                        copy_state.punch.y += (state.punch.y < state.pos.by_32(true).y) ? -1 : 1;
                        tile_change(event, std::move(copy_state));
                    }
                }
                punch = false;
            }
            ransuu ransuu;
            u_char apply_damage_value{}; // @note used to change a tile value without using send_tile_update() 

            if (pPeer->clothing[hand] == 2952) // @note check if the user is holding the Digger's Spade
            {
                if(item->id == 2 || item->id == 14)
                {
                    if (block.fg != 0) block.hits[0] = 3;
                    else block.hits[1] = 3;
    
                    float particle_type = 0x02;
    
                    if(item->id == 2) particle_type = ransuu[{0x02, 0x03}]; // @note idk if this is the correct one, at least by looking at the color it looks like dirt
                    else if(item->id == 14) particle_type = ransuu[{0x0e, 0x0f}];
    
                    state_visuals(*event.peer, ::state{
                        .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
                        .pos = state.punch.by_32(),
                        .speed = ::pos{ particle_type, (float)0x61 }
                    });
                }
            }

            switch (item->id)
            {
                case 758: // @note Roulette Wheel
                {
                    u_char number = ransuu[{0, 36}];
                    char color = (number == 0) ? '2' : (ransuu[{0, 3}] < 2) ? 'b' : '4';
                    std::string message = std::format("[`{}{}`` spun the wheel and got `{}{}``!]", pPeer->prefix, pPeer->ltoken[0], color, number);
                    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&pPeer, message](ENetPeer& peer)
                    {
                        packet::create(peer, false, 2000, { "OnTalkBubble", pPeer->netid, message.c_str() });
                        packet::create(peer, false, 2000, { "OnConsoleMessage", message.c_str() });
                    });
                    break;
                }
            }
            switch (item->type)
            {
                case type::STRONG:
                {
                    if (pPeer->role != 2 or pPeer->role != 1) {
                        throw std::runtime_error("It's too strong to break.");
                    }
                    break;
                } 
                case type::MAIN_DOOR: throw std::runtime_error("(stand over and punch to use)");
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @todo seperate area for 'range_lock'
                    
                    if (world->owner != pPeer->user_id)
                        throw std::runtime_error(std::format("`5[```w{}`` `$World Locked`` by (null)`5]``", world->name)); // @todo add owner name
                    break;
                }
                case type::PROVIDER:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick)
                    {
                        switch (item->id)
                        {
                            case 1008: // @note ATM
                            {
                                u_char gems = ransuu[{1, 100}]; // @note source: https://growtopia.fandom.com/wiki/ATM_Machine
                                for (short i : {100, 50, 10, 5, 1}/* gem type */)
                                    for (; gems >= i; gems -= i/* downgrade type */)
                                        item_change_object(event, {112, static_cast<u_short>(i)}, {
                                            state.punch.by_32().x + ransuu[{0, 16}],
                                            state.punch.by_32().y + ransuu[{0, 16}]
                                        }, -1 /* keep gem colors split instead of merging */);
                                        
                                break;
                            }
                            case 872:/*chicken*/ case 866:/*cow*/ case 1632:/*coffee maker*/ case 3888:/*sheep*/
                            {
                                add_drop(event, ::slot(item->id+2, ransuu[{1, 2}]), state.punch.by_32());
                                break;
                            }
                            case 5116:/*tea set*/
                            {
                                add_drop(event, ::slot(item->id-2, ransuu[{1, 2}]), state.punch.by_32());
                                break;
                            }
                            case 2798:/*well*/
                            {
                                add_drop(event, ::slot(822/*water bucket*/, ransuu[{1, 2}]), state.punch.by_32());
                                break;
                            }
                            case 928:/*science station*/ // @note source: https://growtopia.fandom.com/wiki/Science_Station
                            {
                                short chemcial = 
                                    (!ransuu[{0, 16}]) ? chemcial = 918/*P*/ : 
                                    (!ransuu[{0, 8}])  ? chemcial = 920/*B*/ : 
                                    (!ransuu[{0, 6}])  ? chemcial = 924/*Y*/ : 
                                    (!ransuu[{0, 4}])  ? chemcial = 916/*R*/ : chemcial = 914/*G*/;
                                add_drop(event, {chemcial, 1}, state.punch.by_32());
                                break;
                            }
                        }
                        block.tick = steady_clock::now();
                        send_tile_update(event, std::move(state), block, *world); // @note update countdown on provider.

                        pPeer->add_xp(event, 1);
                        return;
                    }
                    break;
                }
                case type::SEED:
                {
                    if ((steady_clock::now() - block.tick) / 1s >= item->tick) // @todo limit this check.
                    {
                        block.hits[0] = 99;
                        add_drop(event, ::slot(item->id - 1, ransuu[{2, 12}]), state.punch.by_32()); // @note fruit (from tree)
                    }
                    break;
                }
                case type::WEATHER_MACHINE:
                {
                    ::block &weather_machine = world->blocks[cord(world->现weather.x, world->现weather.y)];

                    if (!(block.state[2] & S_TOGGLE) && !(weather_machine.state[2] & S_TOGGLE)) weather_machine.state[2] &= ~S_TOGGLE; // @note so we can avoid the upcoming ^= if the weather machine is already toggled
                    block.state[2] ^= S_TOGGLE; // @note if punched twice it can detoggle that is why we use ^= not |=
                    
                    world->现weather = state.punch;
                    
                    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [block, item](ENetPeer& p)
                    {
                        packet::create(p, false, 0, { "OnSetCurrentWeather", (block.state[2] & S_TOGGLE) ? get_weather_id(item->id) : 0 });
                    });
                    break;
                }
                case type::TOGGLEABLE_BLOCK:
                case type::TOGGLEABLE_ANIMATED_BLOCK:
                case type::CHEST:
                {
                    block.state[2] ^= S_TOGGLE;
                    if (item->id == 226) // @note Signal Jammer
                    {
                        packet::create(*event.peer, false, 0, {
                            "OnConsoleMessage",
                            (block.state[2] & S_TOGGLE) ? 
                                "Signal jammer enabled. This world is now `4hidden`` from the universe." :
                                "Signal jammer disabled.  This world is `2visible`` to the universe."
                        });
                    }
                    break;
                }
                case type::RANDOM:
                {
                    apply_damage_value = 
                        (item->id == 456/*Dice*/) ? ransuu[{0, 5}] : 
                        (item->id == 1300/*Roshambo*/) ? ransuu[{1, 3}] : 0;

                    auto random = std::ranges::find(world->random_blocks, state.punch, &::random_block::pos);
                    if (random == world->random_blocks.end())
                    {
                        world->random_blocks.emplace_back(::random_block{apply_damage_value, state.punch});
                    }
                    else random->value = apply_damage_value;
                    break;
                }
            }
            tile_apply_damage(event, std::move(state), block, apply_damage_value);

            if (block.hits[0] >= item->hits) block.fg = 0, block.hits[0] = 0;
            else if (block.hits[1] >= item->hits) block.bg = 0, block.hits[1] = 0;
            else return;

            if (item->type == type::VENDING_MACHINE)
            {
                auto vend = std::ranges::find(world->vendings, state.punch, &::vending_machine::pos);
                if (vend != world->vendings.end())
                {
                    if (vend->item_id != 0 && vend->stock > 0)
                        for (int left = vend->stock; left > 0; left -= 200)
                            add_drop(event, {vend->item_id, static_cast<short>(std::min(left, 200))}, state.punch.by_32());
                    world->vendings.erase(vend);
                }
            }
            
            /* @todo update these changes with tile_update() */
            block.label = "";
            block.state[2] = 0x00; // @note reset tile direction
            block.state[3] &= ~S_VANISH; // @note remove paint
            
            { /* gem drop on any successful block break */
                short rarity = std::max<short>(0, item->rarity);
                u_char max_gems =
                    static_cast<u_char>(std::min<short>(150, 1 + rarity));
                u_char min_gems = static_cast<u_char>(
                    rarity > 38 ? std::min<int>(20, max_gems) : 1
                );

                if (!ransuu[{0, (rarity >= 50) ? 1 : (rarity >= 15) ? 2 : 3}])
                {
                    u_char gems = ransuu[{min_gems, max_gems}];
                    for (short i : {100, 50, 10, 5, 1})
                        for (; gems >= i; gems -= i)
                            item_change_object(event, {112, static_cast<u_short>(i)}, {
                                state.punch.by_32().x + ransuu[{0, 16}],
                                state.punch.by_32().y + ransuu[{0, 16}]
                            }, -1 /* keep gem colors split instead of merging */);
                }
            }

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

                add_drop(event, ::slot(reward, (reward == 3408 || reward == 3404) ? 10 : 1), state.punch.by_32());
                if (reward == 1458)
                {
                    std::string message = std::format("msg|`4The Power of Love! `2{} found a `#Golden Heart Crystal`2 in a `#{}`2!", pPeer->ltoken[0], item->raw_name);
                    peers(pPeer->recent_worlds.back(), PEER_ALL, [message](ENetPeer &p)
                    {
                        packet::action(p, "log", message.c_str());
                    });
                }
                if (++pPeer->gbc_pity % 100 == 0) modify_item_inventory(event, ::slot{9350, 1});
            }
            else if (item->type == type::LOCK && !is_tile_lock(item->id))
            {
                if (!pPeer->role)
                {
                    pPeer->prefix.front() = 'w';
                    on::NameChanged(event);
                }
                
                world->owner = 0; // @todo have a seperate thing for 'range_lock'
            }

            if (item->cat == CAT_RETURN)
            {
                int uid = item_change_object(event, ::slot(item->id, 1), state.pos);
                item_activate_object(event, ::state{.id = uid, .punch = state.punch});
            }
            else if (u_char(item->property) & 04) { } // @note "This item never drops any seeds."; should it drop a block?
            else // @note normal break (drop seed/block & give XP)
            {
                short rarity = std::max<short>(0, item->rarity);
                bool high_rarity = rarity >= 15;

                if (!ransuu[{0, high_rarity ? 2 : 4}]) add_drop(event, ::slot(item->id + 1, 1), state.punch.by_32());
                else if (!ransuu[{0, high_rarity ? 4 : 8}]) add_drop(event, ::slot(item->id, 1), state.punch.by_32());

                pPeer->add_xp(event, static_cast<u_short>(1 + rarity));
            }
        } // @note delete im, id
        else if (item->cloth_type != clothing::none) 
        {
            if (state.punch != pPeer->pos.by_32(true)) throw std::runtime_error("To wear clothing, use on yourself");

            item_activate(event, state);
            return; 
        }
        else if (item->type == type::CONSUMEABLE) 
        {
            if (item->raw_name == "Birth Certificate")
            {
                const long long now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                const long long next_rename_at = pPeer->name_changed_at + (15ll * 24ll * 60ll * 60ll);

                std::string cooldown_text = "`2You can change your name right now.``";
                if (pPeer->name_changed_at != 0 && now < next_rename_at)
                {
                    const long long remaining = next_rename_at - now;
                    const long long days = remaining / 86400;
                    const long long hours = (remaining % 86400) / 3600;
                    cooldown_text = std::format("`4You can change your name again in {} day(s) and {} hour(s).``", days, hours);
                }

                packet::create(*event.peer, false, 0, {
                    "OnDialogRequest",
                    ::create_dialog()
                        .set_default_color("`o")
                        .add_label_with_icon("big", "`wBirth Certificate``", item->id)
                        .add_textbox("Use this to change your GrowID. This consumes 1 Birth Certificate on success.")
                        .add_smalltext(cooldown_text)
                        .add_smalltext("Name rules: 3-20 characters, letters and numbers only.")
                        .add_text_input("name", "New Name", pPeer->ltoken[0], 20)
                        .end_dialog("birth_certificate", "Cancel", "Change Name").c_str()
                });
                return;
            }
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

            if (item->raw_name.contains("Paint Bucket - ") && pPeer->clothing[hand] != 3494) throw std::runtime_error("you need a Paintbrush to apply paint!");
            if (item->raw_name.contains("Hair Dye"))
            {
                if (state.punch != pPeer->pos.by_32(true)) throw std::runtime_error("Don't spill your dye!");
                else if (world->blocks[cord(pPeer->pos.by_32(true).x, pPeer->pos.by_32(true).y)].fg != 230/*Bathtub*/) throw std::runtime_error("You'll make a huge mess if you do that outside the Bathtub!");

                on::Action(event, "shower");
                // audio/shower.wav
                packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer->netid, "You dyed your hair!", 0u, 1u });
            }
            float color{}; // @note the color of the paint particle effect.
            float particle{};
            switch (item->id)
            {
                case 1404: // @note Door Mover
                {
                    if (!door_mover(*world, state.punch)) throw std::runtime_error("There's no room to put the door there! You need 2 empty spaces vertically.");

                    std::string remember_name = world->name;
                    std::vector<ENetPeer*> world_peers = peers(remember_name, PEER_SAME_WORLD);

                    for (ENetPeer *peer : world_peers)
                    {
                        if (!peer || peer->state != ENET_PEER_STATE_CONNECTED) continue;

                        ENetEvent peer_event{};
                        peer_event.peer = peer;
                        action::quit_to_exit(peer_event, "", true); // @note everyone in world exits
                    }

                    for (ENetPeer *peer : world_peers)
                    {
                        if (!peer || peer->state != ENET_PEER_STATE_CONNECTED) continue;

                        ENetEvent peer_event{};
                        peer_event.peer = peer;
                        action::join_request(peer_event, "", remember_name); // @note everyone in world re-joins
                    }
                    return;
                }
                case 822: // @note Water Bucket
                {
                    if (block.state[3] & S_FIRE) remove_fire(event, state, block, *world);
                    else block.state[3] ^= S_WATER;
                    break;
                }
                case 1866: // @note Block Glue
                {
                    block.state[3] ^= S_GLUE;
                    break;
                }
                case 3062: // @note Pocket Lighter
                {
                    if (block.fg == 0 && block.bg == 0) throw std::runtime_error("There's nothing to burn!");
                    if (block.state[3] & (S_FIRE | S_WATER)) return; // @note avoid fire on water & fire on fire

                    block.state[3] |= S_FIRE;

                    std::string message = "`7[```4MWAHAHAHA!! FIRE FIRE FIRE```7]``";
                    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
                    {
                        packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer->netid, message.c_str(), 0u });
                        packet::create(*event.peer, false, 0, { "OnConsoleMessage", message.c_str() });
                    });
                    particle = 0x96;

                    if (block.fg == 3090) // @note Highly Combustible Box
                    {
                        block.fg = 3128; // @note Combusted Box
                        if (!(block.state[2] & S_TOGGLE)/*closed*/) {} // @todo recipes: https://growtopia.fandom.com/wiki/Guide:Highly_Combustible_Box
                    }
                    break;
                }
                case 3404:/*Sour Lollipop*/ case 3406:/*Sweet Lollipop*/
                {
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer->netid, "`#YUM!:D", 0u });

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
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer->netid, "`#GULP! You got smarter!", 0u });
                    pPeer->add_xp(event, 10000);
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
                    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
                    {
                        ::peer *_p = static_cast<::peer*>(p.data);

                        if (state.punch == _p->pos.by_32(true))
                        {
                            _p->state |= S_DUCT_TAPE; // @todo add a 10 minute timer that will remove it.
                            on::SetClothing(p);
                        }
                    });
                    break;
                }
                case 3478: // @note Paint Bucket - Red
                {
                    block.state[3] |= S_RED;
                    color = 0x0000ff00, particle = 0xa8; 
                    break;
                }
                case 3480: // @note Paint Bucket - Yellow
                {
                    block.state[3] |= S_YELLOW;
                    color = 0x00ffff00, particle = 0xa8; // @note red + green
                    break;
                }
                case 3482: // @note Paint Bucket - Green
                {
                    block.state[3] |= S_GREEN;
                    color = 0x00ff0000, particle = 0xa8;
                    break;
                }
                case 3484: // @note Paint Bucket - Aqua
                {
                    block.state[3] |= S_AQUA;
                    color = 0xffff0000, particle = 0xa8; // @note blue + green
                    break;
                }
                case 3486: // @note Paint Bucket - Blue
                {
                    block.state[3] |= S_BLUE;
                    color = 0xff000000, particle = 0xa8;
                    break;
                }
                case 3488: // @note Paint Bucket - Purple
                {
                    block.state[3] |= S_PURPLE;
                    color = 0xff00ff00, particle = 0xa8; // @note blue + red
                    break;
                }
                case 3490: // @note Paint Bucket - Charcoal
                {
                    block.state[3] |= S_CHARCOAL;
                    color = 0xffffffff, particle = 0xa8; // @note B(blue)G(green)R(red)A(alpha/opacity) max will provide a pure black color. idk if growtopia is the same.
                    break;
                }
                case 3492: // @note Paint Bucket - Vanish
                {
                    block.state[3] &= ~S_VANISH;
                    color = 0xffffff00, particle = 0xa8; // @todo get exact color. I just guessed T-T
                }
                case 3822: break; // Red Hair Dye
                case 3824: break; // Green Hair Dye
                case 3826: break; // Blue Hair Dye
                case 228: // @note Grow Spray Fertilizer
                {
                    auto tree_item = std::ranges::find(items, (block.fg != 0) ? block.fg : block.bg, &::item::id);
                    if (!apply_tree_growth_spray(event, *pPeer, *world, state, block, *tree_item, 3600, "You spray the tree and it grows `21 hour`` faster!"))
                        return;
                    break;
                }
                case 1778: // @note Deluxe Grow Spray
                {
                    auto tree_item = std::ranges::find(items, (block.fg != 0) ? block.fg : block.bg, &::item::id);
                    if (!apply_tree_growth_spray(event, *pPeer, *world, state, block, *tree_item, 86400, "You spray the tree and it grows `224 hours`` faster!"))
                        return;
                    break;
                }
                default: return; // @note prevent taking the consumeable if nothing happended
            }
            if (particle > 0.0f)
            {
                state_visuals(*event.peer, ::state{
                    .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
                    .pos = state.punch.by_32(),
                    .speed = ::pos{ color, particle }
                });
            }
            send_tile_update(event, std::move(state), block, *world);

            modify_item_inventory(event, ::slot(item->id, -1));
            pPeer->add_xp(event, 1);
            return;
        }
        else if (state.id == 32)
        {
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @todo seperate area for 'range_lock'

                    if (pPeer->user_id == world->owner)
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
                                item->raw_name, item->id, state.punch.x, state.punch.y, to_char(world->is_public), (world->lock_state & DISABLE_MUSIC) ? "1" : "0", world->minimum_entry_level
                            ).c_str()
                        });
                    }
                    break;
                }
                case type::DOOR:
                case type::PORTAL:
                {
                    std::string dest, id{};
                    for (::door& door : world->doors)
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
                            item->raw_name, item->id, to_char((block.state[2] & S_PUBLIC)), state.punch.x, state.punch.y
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
                            .add_label_with_icon("big", std::format("`w{}``", item->raw_name), item->id)
                            /* @todo complete */
                    });
                    break;
                }
                case type::VENDING_MACHINE:
                {
                    if (pPeer->pos.by_32(true) != state.punch) throw std::runtime_error("Stand on the vending machine to use it.");
                    show_vending_dialog(event, *world, *pPeer, state.punch);
                    break;
                }
            }
            return; // @note leave early else wrench will act as a block unlike fist which breaks. this is cause of state_visuals()
        }
        else // @note placing a block
        {
            if (block.fg != 0) // @note placing something ontop of exisitng block
            {
                bool update_tile{};
                switch (items[world->blocks[cord(state.punch.x, state.punch.y)].fg].type)
                {
                    case type::DISPLAY_BLOCK:
                    {
                        world->displays.emplace_back(::display(item->id, state.punch));
                        update_tile = true;
                        break;
                    }
                    case type::SEED:
                    {
                        for (::item &item : items)
                        {
                            if ((item.splice[0] == state.id && item.splice[1] == block.fg) ||
                                (item.splice[1] == state.id && item.splice[0] == block.fg) /* allow reverse splice combo */)
                            {
                                auto splice0 = std::ranges::find(items, item.splice[0], &::item::id);
                                auto splice1 = std::ranges::find(items, item.splice[1], &::item::id);

                                packet::create(*event.peer, false, 0, {
                                    "OnTalkBubble", 
                                    pPeer->netid, 
                                    std::format("`w{}`` and `w{}`` have been spliced to make a `${} Tree``!", 
                                        splice0->raw_name, splice1->raw_name, item.raw_name.substr(0, item.raw_name.length()-5/* seed*/)).c_str(), // @todo this is hardcoded
                                    0u,
                                    1u
                                });
                                block.tick = steady_clock::now();
                                block.fg = item.id;
                                update_tile = true;
                                break;
                            }
                        }
                        break;
                    }
                }
                if (update_tile)
                    send_tile_update(event, std::move(state), block, *world);
                return;
            }
            if (item->collision == collision::FULL)
            {
                if (state.punch == state.pos.by_32(true)) return; // @todo when moving avoid collision.
            }
            switch (item->type)
            {
                case type::LOCK:
                {
                    if (is_tile_lock(item->id)) break; // @note seperate area for 'range_lock'

                    if (!world->owner)
                    {
                        world->owner = pPeer->user_id;
                        lock_visuals = true;
                        if (!pPeer->role) 
                        {
                            pPeer->prefix.front() = '2';
                            on::NameChanged(event);
                        }
                        if (std::ranges::find(pPeer->my_worlds, world->name) == pPeer->my_worlds.end()) 
                        {
                            std::ranges::rotate(pPeer->my_worlds, pPeer->my_worlds.begin() + 1);
                            pPeer->my_worlds.back() = world->name;
                        }
                        std::string placed_message = std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world->name, pPeer->ltoken[0]);
                        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&pPeer, placed_message](ENetPeer& peer) 
                        {
                            packet::create(peer, false, 0, { "OnTalkBubble", pPeer->netid, placed_message.c_str() });
                            packet::create(peer, false, 0, { "OnConsoleMessage", placed_message.c_str() });
                        });
                    }
                    else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
                    break;
                }
                case type::ENTRANCE:
                {
                    block.state[2] |= S_PUBLIC;
                    break;
                }
                case type::PROVIDER:
                {
                    block.tick = steady_clock::now();
                    break;
                }
                case type::SEED:
                {
                    block.state[2] |= 0x11;
                    block.tick = steady_clock::now();
                    break;
                }
            }
            block.state[2] |= (pPeer->facing_left) ? S_LEFT : S_RIGHT;
            (item->type == type::BACKGROUND) ? block.bg = state.id : block.fg = state.id;
            pPeer->emplace(::slot(item->id, -1));
        }
        state.netid = pPeer->netid; // @todo sometimes rgt has this as 0
        state_visuals(*event.peer, std::move(state)); // finished.
        if (lock_visuals) 
        {
            state_visuals(*event.peer, ::state{
                .type = 0x0f, // @note PACKET_SEND_LOCK
                .netid = world->owner, 
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
                pPeer->netid, 
                exc.what(),
                0u,
                1u // @note message will be sent once instead of multiple times.
            });
        return;
    }
}
