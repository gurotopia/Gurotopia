#include "pch.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "on/Spawn.hpp"
#include "on/BillboardChange.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"
#include "on/ConsoleMessage.hpp"
#include "commands/weather.hpp"
#include "tools/ransuu.hpp"

#include "join_request.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);

        std::string big_name{header.size() < 2 ? world_name : readch(header, '|')[3]};
        if (!alnum(big_name)) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        
        auto it = std::ranges::find(worlds, big_name, &::world::name);
        if (it == worlds.end()) 
            it = worlds.emplace(it, big_name);
            
        ::world &world = *it;

        std::vector<std::string> buffs{};
        {
            std::vector<u_char> data = compress_state(::state{
                .type = 0x04, // @note PACKET_SEND_MAP_DATA
                .peer_state = peer_state::S_EXTENDED
            });
            data.resize(data.size() + 24ull + world.name.length() + (8ull * world.blocks.size()) + 12ull + 8ull/*total drop uid*/);
            u_char *w_data = data.data() + sizeof(::state) + 6;

            const short len = world.name.length();
            *reinterpret_cast<short*>(w_data) = len; w_data += sizeof(short);
            for (u_char c : world.name) *w_data++ = c;
            
            const int y = world.blocks.size() / 100;
            const int x = world.blocks.size() / y;
            *reinterpret_cast<int*>(w_data) = x; w_data += sizeof(int);
            *reinterpret_cast<int*>(w_data) = y; w_data += sizeof(int);
            *reinterpret_cast<u_short*>(w_data) = static_cast<u_short>(world.blocks.size()); w_data += sizeof(u_short);
            w_data += 7; // @todo

            for (u_short i = 0; const ::block &block : world.blocks)
            {
                *reinterpret_cast<short*>(w_data) = block.fg; w_data += sizeof(short);
                *reinterpret_cast<short*>(w_data) = block.bg; w_data += sizeof(short);

                w_data += sizeof(short);
                *w_data++ = block.state[2];
                *w_data++ = block.state[3];

                int offset = w_data - data.data();
                const ::item &item = id_to_item(block.fg); // @todo limit iteration during world enter
                switch (item.type)
                {
                    case type::TRAMPOLINE:
                    case type::ENTRANCE:
                    case type::SFX_BLOCK: // @note roulette wheel
                    case type::PLATFORM:
                    case type::STRONG: // @note bedrock
                    case type::FIRE_PAIN: // @note lava
                    case type::FOREGROUND: 
                    case type::BACKGROUND:
                    case type::ANIMATED: // @note chand
                    case type::BOUNCY:
                    case type::CHECKPOINT:
                    case type::TOGGLEABLE_BLOCK:
                    case type::CHEST: // @note treasure, booty chest
                        break;
                    case type::LOCK: 
                    {
                        if (!is_tile_lock(block.fg)) world.is_public = (block.state[2] & S_PUBLIC); // @note check if world lock has S_PUBLIC flag, i will change this later

                        int access = std::ranges::count_if(world.access, std::identity{});
                        data.resize(data.size() + 1ull + 1ull + 4ull + 4ull + (access * 4ull));
                        w_data = data.data() + offset;

                        *w_data++ = 0x03;
                        *w_data++ = world.lock_state;
                        *reinterpret_cast<int*>(w_data) = world.owner; w_data += sizeof(int);
                        *reinterpret_cast<int*>(w_data) = access; w_data += sizeof(int);
                        // @todo minimal level
                        /* @todo access list */
                        break;
                    }
                    case type::MAIN_DOOR: 
                    {
                        pPeer->rest_pos = ::pos((i % x) * 32 , (i / x) * 32);

                        [[fallthrough]];
                    }
                    case type::DOOR:
                    case type::PORTAL:
                    {
                        const short len = block.label.length();
                        data.resize(data.size() + 4ull + len); // @note 01 {2} {} 0 0
                        w_data = data.data() + offset;

                        *w_data++ = 0x01;

                        *reinterpret_cast<short*>(w_data) = len; w_data += sizeof(short);
                        for (u_char c : block.label) *w_data++ = c;
                        *w_data++ = '\0'; // @note terminator which Growtopia requires.
                        break;
                    }
                    case type::MAILBOX:
                    {
                        // @todo add "full" label (not here, but in tile_change.cpp)

                        [[fallthrough]];
                    }
                    case type::SIGN:
                    {
                        const short len = block.label.length();
                        data.resize(data.size() + 1ull + 2ull + len + 4ull); // @note 02 {2} {} ff ff ff ff
                        w_data = data.data() + offset;

                        *w_data++ = 0x02;

                        *reinterpret_cast<short*>(w_data) = len; w_data += sizeof(short);
                        for (u_char c : block.label) *w_data++ = c;
                        *reinterpret_cast<int*>(w_data) = 0xffffffff; w_data += sizeof(int); // @note ff ff ff ff
                        break;
                    }
                    case type::SEED:
                    {
                        data.resize(data.size() + 1ull + 4ull + 1ull);
                        w_data = data.data() + offset;

                        *w_data++ = 0x04;

                        *reinterpret_cast<u_int*>(w_data) = (steady_clock::now() - block.tick) / 1s; w_data += sizeof(u_int);
                        *w_data++ = 0x03; // @note fruit on tree
                        break;
                    }
                    case type::PROVIDER:
                    {
                        data.resize(data.size() + 1ull + 4ull);
                        w_data = data.data() + offset;

                        *w_data++ = 0x09;

                        *reinterpret_cast<u_int*>(w_data) = (steady_clock::now() - block.tick) / 1s; w_data += sizeof(u_int);
                        break;
                    }
                    case type::WEATHER_MACHINE:
                    {
                        if (block.state[2] & S_TOGGLE)
                        {
                            send_varlist(event.peer, { "OnSetCurrentWeather", get_weather_id(block.fg) });
                        }
                        break;
                    }
                    case type::TOGGLEABLE_ANIMATED_BLOCK:
                    {
                        if (block.state[2] & S_TOGGLE)
                        {
                            if (block.fg == 226 && std::ranges::find(buffs, "`4JAMMED") == buffs.end()) 
                                buffs.emplace_back("`4JAMMED");
                            else if (block.fg == 1276 && std::ranges::find(buffs, "`2NOPUNCH") == buffs.end())
                                buffs.emplace_back("`2NOPUNCH");
                            else if (block.fg == 1278 && std::ranges::find(buffs, "`2IMMUNE") == buffs.end())
                                buffs.emplace_back("`2IMMUNE");
                            else if (block.fg == 4992 && std::ranges::find(buffs, " `2ANTIGRAVITY") == buffs.end())
                                buffs.emplace_back(" `2ANTIGRAVITY");
                        }
                        break;
                    }
                    case RANDOM:
                    {
                        data.resize(data.size() + 1ull + 1ull);
                        w_data = data.data() + offset;
                        *w_data++ = 0x08;

                        auto random = std::ranges::find(world.random_blocks, ::pos{i % x, i / x}, &::random_block::pos);
                        if (random == world.random_blocks.end()) 
                        {
                            world.random_blocks.emplace_back(::random_block{0, ::pos{i % x, i / x}});
                            *w_data++ = 0;
                        }
                        else {
                            *w_data++ = random->value;
                        }
                        break;
                    }
                    case DISPLAY_BLOCK:
                    {
                        data.resize(data.size() + 1ull + 4ull);
                        w_data = data.data() + offset;
                        *w_data++ = 0x17;
                        
                        auto display = std::ranges::find(world.displays, ::pos{i % x, i / x}, &::display::pos);
                        if (display == world.displays.end()) 
                        {
                            world.displays.emplace_back(::display{0, ::pos{i % x, i / x}});
                            *reinterpret_cast<int*>(w_data) = 0; w_data += sizeof(int);
                        }
                        else {
                            *reinterpret_cast<int*>(w_data) = display->id; w_data += sizeof(int);
                        }
                        break;
                    }
                    case type::VENDING_MACHINE:
                    {
                        data.resize(data.size() + 1ull + 4ull + 4ull);
                        w_data = data.data() + offset;

                        *w_data++ = 0x18;
                        *reinterpret_cast<u_int*>(w_data) = 0; w_data += sizeof(u_int); // @note item's ID
                        *reinterpret_cast<int*>(w_data) = 0; w_data += sizeof(int); // @note world locks per item (or item(s) per world lock)

                        break;
                    }
                    case type::FISH_TANK_PORT:
                    {
                        data.resize(data.size() + 1ull);
                        w_data = data.data() + offset;

                        *w_data++ = 0x00; // @todo if glow toggled this becomes 0x10
                        break;
                    }
                    default: 
                        throw std::runtime_error(std::format("`w{}``'s visuals has not been added yet. ({})", 
                            item.raw_name, item.type));
                }
                ++i;
            }
            w_data += 12; // @todo

            *reinterpret_cast<u_int*>(w_data) = world.last_object_uid; w_data += sizeof(u_int);
            *reinterpret_cast<u_int*>(w_data) = world.last_object_uid; w_data += sizeof(u_int);
            for (const ::object &object : world.objects) 
            {
                int offset = w_data - data.data();
                data.resize(data.size() + sizeof(::object) + 4ull/*@todo*/);
                w_data = data.data() + offset;
                
                *reinterpret_cast<u_short*>(w_data) = object.id;        w_data += sizeof(u_short);
                *reinterpret_cast<float*>(w_data)   = object.pos.x; w_data += sizeof(float);
                *reinterpret_cast<float*>(w_data)   = object.pos.y; w_data += sizeof(float);
                *reinterpret_cast<u_short*>(w_data) = object.count;     w_data += sizeof(u_short);
                *reinterpret_cast<u_int*>(w_data)   = object.uid;       w_data += sizeof(u_int);
            }
            enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
        } // @note delete data
        {
            std::string *w_name = std::ranges::find(pPeer->recent_worlds, world.name);
            std::string *first = w_name != pPeer->recent_worlds.end() ? w_name : pPeer->recent_worlds.begin();

            std::rotate(first, first + 1, pPeer->recent_worlds.end());
            pPeer->recent_worlds.back() = world.name;
        } // @note delete name, first
        on::EmoticonDataChanged(event);

        if (!pPeer->role)
            pPeer->prefix.front() = 
                (pPeer->user_id == world.owner) ? '2' : 
                (std::ranges::find(world.access, pPeer->user_id) != world.access.end()) ? 'c' : 
                pPeer->prefix.front(); // @note keeps the existing prefix

        pPeer->pos = pPeer->rest_pos;
        pPeer->netid = ++world.netid_counter;
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer, &world](ENetPeer& peer/*send to everyone in world*/) 
        {
            ::peer *pOthers = static_cast<::peer*>(peer.data); // @note everyone in world's peer.data
            
            if (pOthers->user_id != pPeer->user_id)
            {
                on::Spawn(*event.peer, pOthers->netid, pOthers->user_id, pOthers->pos, std::format("`{}{}", pOthers->prefix, pOthers->growid), pOthers->country, pOthers->role, pOthers->role >= DEVELOPER, false);
                on::Spawn(peer, pPeer->netid, pPeer->user_id, pPeer->pos, std::format("`{}{}", pPeer->prefix, pPeer->growid), pPeer->country, pPeer->role, pPeer->role >= DEVELOPER, false);
                on::SetClothing(peer);
                on::ConsoleMessage(&peer, std::format("`5<`{}{}`` entered, `w{}`` others here>``", pPeer->prefix, pPeer->growid, world.visitors));
            }
            

            if (pOthers->user_id != pPeer->user_id) // @note the reason this is here is cause we need the peer's OnSpawn to happen before OnTalkBubble
            {
                send_varlist(&peer, {
                    "OnTalkBubble",
                    pPeer->netid,
                    std::format("`5<`{}{}`` entered, `w{}`` others here>``", pPeer->prefix, pPeer->growid, world.visitors),
                    1u
                });
            }
        });
        on::Spawn(*event.peer, pPeer->netid, pPeer->user_id, pPeer->pos, std::format("`{}{}", pPeer->prefix, pPeer->growid), pPeer->country, pPeer->role, pPeer->role >= DEVELOPER, true);

        if (pPeer->billboard.id != 0) on::BillboardChange(event); // @note don't waste memory if billboard is empty.

        send_varlist(event.peer, {
            "OnSetPos", 
            CL_Vec2f{pPeer->rest_pos.x, pPeer->rest_pos.y}
        }, pPeer->netid);

        on::ConsoleMessage(event.peer, 
            std::format(
                "World `w{}{}`` entered.  There are `w{}`` other people here, `w{}`` online.", 
                world.name, (buffs.empty()) ? "" : std::format(" `0[``{}`0]``", join(buffs, "``, ")), world.visitors, peers().size()
            )
        );
        ++world.visitors;
        on::SetClothing(*event.peer);
        on::CountryState(event);
    }
    catch (const std::exception& exc)
    {
        send_varlist(event.peer, { "OnFailedToEnterWorld" });
        return;
    }
}