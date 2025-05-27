#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "punch.hpp"

#include "tools/randomizer.hpp"

void punch(ENetEvent event, state state) 
{
    try
    {
        if (not create_rt(event, 0, 160)) return;
        short block1D = state.punch[1] * 100 + state.punch[0]; // 2D (x, y) to 1D ((destY * y + destX)) formula
        world &world = worlds[_peer[event.peer]->recent_worlds.back()];
        block &b = world.blocks[block1D];
        if (state.id == 18) // @note punching a block
        {
            if (b.bg == 0 && b.fg == 0) return;
            if (items[b.fg].type == std::byte{ type::STRONG }) throw std::runtime_error("It's too strong to break.");
            if (items[b.fg].type == std::byte{ type::MAIN_DOOR }) throw std::runtime_error("(stand over and punch to use)");
            block_punched(event, state, b);
            short id{};
            if (b.fg != 0 && b.hits[0] >= items[b.fg].hits) id = b.fg, b.fg = 0;
            else if (b.bg != 0 && b.hits[1] >= items[b.bg].hits) id = b.bg, b.bg = 0;
            else return;
            std::array<short, 2ull> im{};
            if (not randomizer(0, 7)) im = {112, 1}; // @todo get real growtopia gem drop amount.
            if (not randomizer(0, 13)) im = {id, 1};
            if (not randomizer(0, 9)) im = {++id, 1};
            if (not im.empty())
                drop_visuals(event, 
                    im,
                    {
                        static_cast<float>(state.punch[0]) + randomizer(0.05f, 0.1f), 
                        static_cast<float>(state.punch[1]) + randomizer(0.05f, 0.1f)
                    }
                );
            _peer[event.peer]->add_xp(std::trunc(1.0f + items[id].rarity / 5.0f));
        } // @note delete im, id
        else if (items[state.id].cloth_type != clothing::none) return;
        else if (state.id == 32)
        {
            switch (items[b.fg].type)
            {
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
                        "end_dialog|door_edit|Cancel|OK|", items[b.fg].raw_name, b.fg, b.label, state.punch[0], state.punch[1]).c_str()
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
                        "end_dialog|sign_edit|Cancel|OK|", items[b.fg].raw_name, b.fg, b.label, state.punch[0], state.punch[1]).c_str()
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
                        "end_dialog|gateway_edit|Cancel|OK|", items[b.fg].raw_name, b.fg, state.punch[0], state.punch[1]).c_str()
                    });
                    break;
            }
            return; // @note leave early else wrench will act as a block unlike fist which breaks. this is cause of state_visuals()
        }
        else // @note placing a block
        {
            if (items[state.id].type == std::byte{ type::LOCK })
            {
                if (world.owner == 00)
                {
                    world.owner = _peer[event.peer]->user_id;
                    peers(ENET_PEER_STATE_CONNECTED, [&](ENetPeer& p) 
                    {
                        if (!_peer[&p]->recent_worlds.empty() && !_peer[event.peer]->recent_worlds.empty() &&
                            _peer[&p]->recent_worlds.back() == _peer[event.peer]->recent_worlds.back()) 
                        {
                            const char* placed_message = std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world.name, _peer[event.peer]->ltoken[0]).c_str();
                            gt_packet(p, false, 0, {
                                "OnTalkBubble", 
                                _peer[event.peer]->netid,
                                placed_message,
                                0u
                            });
                            gt_packet(p, false, 0, {
                                "OnConsoleMessage",
                                placed_message
                            });
                        }
                    });
                    gt_packet(*event.peer, true, 0, {
                       "OnNameChanged",
                        std::format("`2{}``", _peer[event.peer]->ltoken[0]).c_str()
                    });
                }
                else throw std::runtime_error("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.");
            }
            if (items[state.id].collision == collision::full)
            {
                // 이 (left, right)
                bool x = state.punch.front() == std::lround(state.pos.front() / 32);
                // 으 (up, down)
                bool y = state.punch.back() == std::lround(state.pos.back() / 32);

                // @note because floats are rounded weirdly in Growtopia...
                bool x_nabor = state.punch.front() == std::lround(state.pos.front() / 32) + 1;
                bool y_nabor = state.punch.back() == std::lround(state.pos.back() / 32) + 1;

                bool collision = (x && y) || (x_nabor && y_nabor);
                if ((_peer[event.peer]->facing_left && collision) || 
                    (not _peer[event.peer]->facing_left && collision)) return;
            }
            (items[state.id].type == std::byte{ type::BACKGROUND }) ? b.bg = state.id : b.fg = state.id; // @note this helps prevent foregrounds to act as backgrounds.
            _peer[event.peer]->emplace(slot{
                static_cast<short>(state.id),
                -1 // @note remove that item the peer just placed.
            });
        }
        state_visuals(event, std::move(state)); // finished.
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) 
            gt_packet(*event.peer, false, 0, {
                "OnTalkBubble", 
                _peer[event.peer]->netid,
                exc.what()
            });
    }
}