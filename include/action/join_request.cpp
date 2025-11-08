#include "pch.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "on/BillboardChange.hpp"
#include "on/SetClothing.hpp"
#include "commands/weather.hpp"
#include "tools/ransuu.hpp"
#include "tools/string.hpp"
#include "join_request.hpp"

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif
using namespace std::literals::chrono_literals;

void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        auto &peer = _peer[event.peer];

        std::string big_name{world_name.empty() ? readch(header, '|')[3] : world_name};
        if (!alnum(big_name)) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        
        auto [it, inserted] = worlds.try_emplace(big_name, big_name);
        ::world &world = it->second; // @note ::world will load from SQL if found. next line, if not.
        if (world.name.empty()) generate_world(world, big_name); // @note make a new world if not found.

        std::vector<std::string> buffs{};
        {
            std::vector<std::byte> data(85 + world.name.length() + 5/*unknown*/ + (8 * world.blocks.size()) + 12 + 8/*total drop uid*/, std::byte{ 00 });
            data[0zu] = TYPE_PACKET;
            data[4zu] = std::byte{ 04 }; // @note PACKET_SEND_MAP_DATA
            data[16zu] = PACKET_STATE;
            
            u_char len = static_cast<u_char>(world.name.length());
            data[66zu] = std::byte{ len };

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(world.name.data());
            for (u_char i = 0; i < len; ++i)
                data[68zu + i] = _1bit[i];

            u_int y = world.blocks.size() / 100;
            u_int x = world.blocks.size() / y;
            *reinterpret_cast<u_int*>(&data[68zu + len]) = x;
            *reinterpret_cast<u_int*>(&data[72zu + len]) = y;
            *reinterpret_cast<u_short*>(&data[76zu + len]) = static_cast<u_short>(world.blocks.size());
            
            std::size_t pos = 85 + len;
            short i = 0;
            for (const block &block : world.blocks)
            {
                *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
                *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
                pos += sizeof(short);

                data[pos++] = std::byte{ block.state3 };
                data[pos++] = std::byte{ block.state4 };
                switch (items[block.fg].type)
                {
                    case type::FOREGROUND: 
                    case type::BACKGROUND:
                        break;

                    case type::LOCK: 
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        std::size_t admins = std::ranges::count_if(world.admin, std::identity{});
                        data.resize(data.size() + 14zu + (admins * 4zu));

                        data[pos++] = std::byte{ 03 };
                        data[pos++] = std::byte{ 00 };
                        *reinterpret_cast<int*>(&data[pos]) = world.owner; pos += sizeof(int);
                        *reinterpret_cast<int*>(&data[pos]) = admins + 1; pos += sizeof(int);
                        *reinterpret_cast<int*>(&data[pos]) = -100; pos += sizeof(int);
                        /* @todo admin list */
                        break;
                    }
                    case type::MAIN_DOOR: 
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        peer->pos.front() = (i % x) * 32;
                        peer->pos.back() = (i / x) * 32;
                        peer->rest_pos = peer->pos;
                        data.resize(data.size() + 8zu);

                        data[pos++] = std::byte{ 01 };
                        *reinterpret_cast<short*>(&data[pos]) = 4; pos += sizeof(short); // @note length of "EXIT"
                        data[pos++] = std::byte{'E'};
                        data[pos++] = std::byte{'X'};
                        data[pos++] = std::byte{'I'};
                        data[pos++] = std::byte{'T'};
                        data[pos++] = std::byte{ 00 }; // @note '\0'
                        break;
                    }
                    case type::ENTRANCE:
                    {
                        data[pos - 2zu] = (block._public) ? std::byte{ 0x90 } : std::byte{ 0x10 };
                        break;
                    }
                    case type::DOOR:
                    case type::PORTAL:
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        short len = block.label.length();
                        data.resize(data.size() + 4zu + len); // @note 01 {2} {} 0 0

                        data[pos++] = std::byte{ 01 };

                        *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
                        for (const char& c : block.label) data[pos++] = static_cast<std::byte>(c);
                        data[pos++] = std::byte{ 00 }; // @note '\0'
                        break;
                    }
                    case type::SIGN:
                    {
                        data[pos - 2zu] = std::byte{ 0x19 };
                        short len = block.label.length();
                        data.resize(data.size() + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff

                        data[pos++] = std::byte{ 02 };

                        *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
                        for (const char& c : block.label) data[pos++] = static_cast<std::byte>(c);
                        *reinterpret_cast<int*>(&data[pos]) = -1; pos += sizeof(int); // @note ff ff ff ff
                        break;
                    }
                    case type::SEED:
                    {
                        data[pos - 2zu] = std::byte{ 0x11 };
                        data.resize(data.size() + 1zu + 5zu);

                        data[pos++] = std::byte{ 04 };
                        *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
                        data[pos++] = std::byte{ 03 }; // @note fruit on tree
                        break;
                    }
                    case type::PROVIDER:
                    {
                        data.resize(data.size() + 5zu);

                        data[pos++] = std::byte{ 0x9 };
                        *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
                        break;
                    }
                    case type::WEATHER_MACHINE: // @note there are no added bytes (I think)
                    {
                        if (block.toggled)
                            packet::create(*event.peer, false, 0, { "OnSetCurrentWeather", get_weather_id(block.fg) });
                        break;
                    }
                    case type::TOGGLEABLE_BLOCK:
                    {
                        if (block.toggled) 
                        {
                            data[pos - 2zu] = std::byte{ 0x50 };
                        }
                        break;
                    }
                    case type::TOGGLEABLE_ANIMATED_BLOCK:
                    {
                        if (block.toggled) 
                        {
                            data[pos - 2zu] = std::byte{ 0x40 };
                            if (block.fg == 226 && std::ranges::find(buffs, "`4JAMMED") == buffs.end()) 
                                buffs.emplace_back("`4JAMMED");
                        }
                        break;
                    }
                    default:
                        data.resize(data.size() + 32zu); // @todo
                        break;
                }
                ++i;
            }
            pos += 12; // @note rgt has it as: bb 7f 06 00 00 00 00 00 5b 0d 0a 00

            *reinterpret_cast<int*>(&data[pos]) = world.ifloat_uid; pos += sizeof(int);
            *reinterpret_cast<int*>(&data[pos]) = world.ifloat_uid; pos += sizeof(int);
            for (const auto& [uid, ifloat] : world.ifloats) 
            {
                data.resize(data.size() + 16zu);
                *reinterpret_cast<short*>(&data[pos]) = ifloat.id; pos += sizeof(short);
                *reinterpret_cast<float*>(&data[pos]) = ifloat.pos[0] * 32.0f; pos += sizeof(float);
                *reinterpret_cast<float*>(&data[pos]) = ifloat.pos[1] * 32.0f; pos += sizeof(float);
                *reinterpret_cast<short*>(&data[pos]) = ifloat.count; pos += sizeof(short);
                *reinterpret_cast<int*>(&data[pos]) = uid; pos += sizeof(int);
            }
            enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
        } // @note delete data
        {
            auto name = std::ranges::find(peer->recent_worlds, world.name);
            auto first = name != peer->recent_worlds.end() ? name : peer->recent_worlds.begin();

            std::rotate(first, first + 1, peer->recent_worlds.end());
            peer->recent_worlds.back() = world.name;
        } // @note delete name, first

        if (peer->user_id == world.owner) peer->prefix.front() = '2';
        else if (std::ranges::find(world.admin, peer->user_id) != world.admin.end()) peer->prefix.front() = 'c';

        peer->netid = ++world.visitors;
        
        peers(event, PEER_SAME_WORLD, [event, &peer, &world](ENetPeer& p) 
        {
            auto &_p = _peer[&p];

            constexpr std::string_view fmt = "spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n{}";
            
            if (_p->user_id != peer->user_id)
            {
                packet::create(*event.peer, false, -1/* ff ff ff ff */, {
                    "OnSpawn", 
                    std::format(fmt, 
                        _p->netid, _p->user_id, static_cast<int>(_p->pos.front()), static_cast<int>(_p->pos.back()), _p->prefix, _p->ltoken[0], (_p->role) ? "1" : "0", (_p->role >= DEVELOPER) ? "1" : "0", 
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
                    peer->netid, peer->user_id, static_cast<int>(peer->pos.front()), static_cast<int>(peer->pos.back()), peer->prefix, peer->ltoken[0], (peer->role) ? "1" : "0", (peer->role >= DEVELOPER) ? "1" : "0", 
                    (_p->user_id == peer->user_id) ? "type|local" : ""
                ).c_str()
            });

            /* the reason this is here is cause we need the peer's OnSpawn to happen before OnTalkBubble */
            if (_p->user_id != peer->user_id)
                packet::create(p, false, 0, {
                        "OnTalkBubble",
                        peer->netid,
                        std::format("`5<`{}{}`` entered, `w{}`` others here>``", peer->prefix, peer->ltoken[0], world.visitors - 1).c_str(),
                        1u
                });
            ENetEvent fake_event{.peer = &p}; // @note used to call functions that take ENetEvent @todo improve!!
            on::SetClothing(fake_event);
        });
        
        inventory_visuals(event);
        if (peer->billboard.id != 0) on::BillboardChange(event); // @note don't waste memory if billboard is empty.

        auto section = [](const auto& range) 
        {
            if (range.empty()) return std::string{};
            
            std::string list{};
            for (const auto& buff : range) 
                list.append(std::format("{}``,", buff));
            list.pop_back();

            return std::format(" `0[``{}`0]``", list);
        };
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage", 
            std::format(
                "World `w{}{}`` entered.  There are `w{}`` other people here, `w{}`` online.", 
                world.name, section(buffs), world.visitors - 1, peers(event).size()
            ).c_str()
        });
        on::EmoticonDataChanged(event);
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) packet::create(*event.peer, false, 0, { "OnConsoleMessage", exc.what() });
        packet::create(*event.peer, false, 0, { "OnFailedToEnterWorld" });
    }
}