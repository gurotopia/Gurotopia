#include "pch.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "on/BillboardChange.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"
#include "commands/weather.hpp"
#include "tools/ransuu.hpp"
#include "tools/string.hpp"
#include "join_request.hpp"

#include <cmath> // @note std::round

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        ::peer *peer = static_cast<::peer*>(event.peer->data);

        std::string big_name{world_name.empty() ? readch(header, '|')[3] : world_name};
        if (!alnum(big_name)) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        
        auto [it, inserted] = worlds.try_emplace(big_name, big_name);
        ::world &world = it->second; // @note ::world will load from SQL if found. next line, if not.
        if (world.name.empty()) generate_world(world, big_name); // @note make a new world if not found.

        std::vector<std::string> buffs{};
        {
            std::vector<u_char> data = compress_state(::state{
                .type = 0x04, // @note PACKET_SEND_MAP_DATA
                .peer_state = 0x08
            });
            data.resize(data.size() + 24zu + world.name.length() + (8zu * world.blocks.size()) + 12zu + 8zu/*total drop uid*/);
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

            u_short i = 0; // @note track the block position
            for (const ::block &block : world.blocks)
            {
                *reinterpret_cast<short*>(w_data) = block.fg; w_data += sizeof(short);
                *reinterpret_cast<short*>(w_data) = block.bg; w_data += sizeof(short);

                w_data += sizeof(short);
                *w_data++ = block.state3;
                *w_data++ = block.state4;

                int offset = w_data - data.data();
                switch (items[block.fg].type)
                {
                    case type::ENTRANCE:
                    case type::FOREGROUND: 
                    case type::BACKGROUND:
                    case type::STRONG: // @note bedrock
                    case type::FIRE_PAIN: // @note lava
                    case type::CHEST: // @note treasure, booty chest
                    case type::TOGGLEABLE_BLOCK:
                    case type::CHECKPOINT:
                        break;
                    case type::LOCK: 
                    {
                        std::size_t admins = std::ranges::count_if(world.admin, std::identity{});

                        data.resize(data.size() + 1zu + 1zu + 4zu + 4zu + 4zu + (admins * 4zu));
                        w_data = data.data() + offset;

                        *w_data++ = 0x03;
                        *w_data++ = 0x00;
                        *reinterpret_cast<int*>(w_data) = world.owner; w_data += sizeof(int);
                        *reinterpret_cast<int*>(w_data) = admins + 1; w_data += sizeof(int);
                        *reinterpret_cast<int*>(w_data) = -100; w_data += sizeof(int);
                        /* @todo admin list */
                        break;
                    }
                    case type::MAIN_DOOR: 
                    {
                        peer->rest_pos = ::pos(i % x, i / x);

                        [[fallthrough]];
                    }
                    case type::DOOR:
                    case type::PORTAL:
                    {
                        const short len = block.label.length();
                        data.resize(data.size() + 4zu + len); // @note 01 {2} {} 0 0
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
                        data.resize(data.size() + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff
                        w_data = data.data() + offset;

                        *w_data++ = 0x02;

                        *reinterpret_cast<short*>(w_data) = len; w_data += sizeof(short);
                        for (u_char c : block.label) *w_data++ = c;
                        *reinterpret_cast<int*>(w_data) = -1; w_data += sizeof(int); // @note ff ff ff ff
                        break;
                    }
                    case type::SEED:
                    {
                        data.resize(data.size() + 1zu + 4zu + 1zu);
                        w_data = data.data() + offset;

                        *w_data++ = 0x04;

                        *reinterpret_cast<int*>(w_data) = (steady_clock::now() - block.tick) / 1s; w_data += sizeof(int);
                        *w_data++ = 0x03; // @note fruit on tree
                        break;
                    }
                    case type::PROVIDER:
                    {
                        data.resize(data.size() + 1zu + 4zu);
                        w_data = data.data() + offset;

                        *w_data++ = 0x09;

                        *reinterpret_cast<int*>(w_data) = (steady_clock::now() - block.tick) / 1s; w_data += sizeof(int);
                        break;
                    }
                    case type::WEATHER_MACHINE:
                    {
                        if (block.state3 & S_TOGGLE)
                            packet::create(*event.peer, false, 0, { "OnSetCurrentWeather", get_weather_id(block.fg) });
                        break;
                    }
                    case type::TOGGLEABLE_ANIMATED_BLOCK:
                    {
                        if (block.state3 & S_TOGGLE)
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
                    case type::FISH_TANK_PORT:
                    {
                        data.resize(data.size() + 1zu);
                        w_data = data.data() + offset;

                        *w_data++ = 0x00; // @todo if glow toggled this becomes 0x10
                        break;
                    }
                    default: 
                        throw std::runtime_error(std::format("`w{}``'s visuals has not been added yet.", 
                            items[block.fg].raw_name));
                }
                ++i;
            }
            w_data += 12; // @todo

            *reinterpret_cast<int*>(w_data) = world.ifloat_uid; w_data += sizeof(int);
            *reinterpret_cast<int*>(w_data) = world.ifloat_uid; w_data += sizeof(int);
            for (const auto &[uid, ifloat] : world.ifloats) 
            {
                const auto &[id, count, pos] = ifloat;
                int offset = w_data - data.data();
                data.resize(data.size() + sizeof(::ifloat) + 4zu/*@todo*/);
                w_data = data.data() + offset;
                
                *reinterpret_cast<short*>(w_data) = id;        w_data += sizeof(short);
                *reinterpret_cast<float*>(w_data) = pos.f_x(); w_data += sizeof(float);
                *reinterpret_cast<float*>(w_data) = pos.f_y(); w_data += sizeof(float);
                *reinterpret_cast<short*>(w_data) = count;     w_data += sizeof(short);
                *reinterpret_cast<int*>(w_data)   = uid;       w_data += sizeof(int);
            }
            enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
        } // @note delete data
        {
            auto name = std::ranges::find(peer->recent_worlds, world.name);
            auto first = name != peer->recent_worlds.end() ? name : peer->recent_worlds.begin();

            std::rotate(first, first + 1, peer->recent_worlds.end());
            peer->recent_worlds.back() = world.name;
        } // @note delete name, first

        if (peer->user_id == world.owner && !peer->role) peer->prefix.front() = '2';
        else if (std::ranges::contains(world.admin, peer->user_id) && !peer->role) peer->prefix.front() = 'c';

        ++world.visitors;
        peer->netid = ++world.netid_counter;
        
        constexpr std::string_view fmt = "spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n{}";
        
        peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &peer, &world, &fmt](ENetPeer& p) 
        {
            ::peer *_p = static_cast<::peer*>(p.data);
            
            if (_p->user_id != peer->user_id)
            {
                packet::create(*event.peer, false, -1/* ff ff ff ff */, {
                    "OnSpawn", 
                    std::format(fmt, 
                        _p->netid, _p->user_id, _p->pos.x, _p->pos.y, _p->prefix, _p->ltoken[0], (_p->role) ? "1" : "0", (_p->role >= DEVELOPER) ? "1" : "0", 
                        ""
                    ).c_str()
                });
                packet::create(p, false, 0, {
                    "OnConsoleMessage",
                    std::format("`5<`{}{}`` entered, `w{}`` others here>``", peer->prefix, peer->ltoken[0], world.visitors - 1).c_str()
                });
            }
            packet::create(p, false, -1/* ff ff ff ff */, {
                "OnSpawn", 
                std::format(fmt,
                    peer->netid, peer->user_id, std::round(peer->rest_pos.f_x()), std::round(peer->rest_pos.f_y()), peer->prefix, peer->ltoken[0], (peer->role) ? "1" : "0", (peer->role >= DEVELOPER) ? "1" : "0", 
                    (_p->user_id == peer->user_id) ? "type|local\n" : ""
                ).c_str()
            });
            on::SetClothing(p);

            /* the reason this is here is cause we need the peer's OnSpawn to happen before OnTalkBubble */
            if (_p->user_id != peer->user_id)
                packet::create(p, false, 0, {
                        "OnTalkBubble",
                        peer->netid,
                        std::format("`5<`{}{}`` entered, `w{}`` others here>``", peer->prefix, peer->ltoken[0], world.visitors - 1).c_str(),
                        1u
                });
        });

        if (peer->billboard.id != 0) on::BillboardChange(event); // @note don't waste memory if billboard is empty.

        auto section = [](const auto& range) 
        {
            if (range.empty()) return std::string{};
            
            std::string list{};
            for (const auto &buff : range) 
                list.append(std::format("{}``, ", buff));
            list.pop_back();
            list.pop_back(); // @note remove the ',' @todo improve

            return std::format(" `0[``{}`0]``", list);
        };
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage", 
            std::format(
                "World `w{}{}`` entered.  There are `w{}`` other people here, `w{}`` online.", 
                world.name, section(buffs), world.visitors - 1, peers().size()
            ).c_str()
        });
        on::EmoticonDataChanged(event);

        on::CountryState(event);
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) packet::create(*event.peer, false, 0, { "OnConsoleMessage", exc.what() });
        packet::create(*event.peer, false, 0, { "OnFailedToEnterWorld" });
    }
}