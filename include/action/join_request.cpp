#include "pch.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "on/BillboardChange.hpp"
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

constexpr std::array<std::byte, 4zu> EXIT{
    std::byte{ 0x45 }, // @note 'E'
    std::byte{ 0x58 }, // @note 'X'
    std::byte{ 0x49 }, // @note 'I'
    std::byte{ 0x54 }  // @note 'T'
};

void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        auto &peer = _peer[event.peer];

        std::string big_name{world_name.empty() ? readch(std::move(header), '|')[3] : world_name};
        if (!alnum(big_name)) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        
        auto [it, inserted] = worlds.try_emplace(big_name, big_name);
        world &world = it->second;
        if (world.name.empty())
        {
            generate_world(world, big_name);
        }
        std::vector<std::string> buffs{};
        {
            std::vector<std::byte> data(85 + world.name.length() + 5/*unknown*/ + (8 * world.blocks.size()) + 12 + 8/*total drop uid*/, std::byte{ 00 });
            data[0zu] = std::byte{ 04 };
            data[4zu] = std::byte{ 04 }; // @note PACKET_SEND_MAP_DATA
            data[16zu] = std::byte{ 0x08 };
            u_char len = static_cast<u_char>(world.name.length());
            data[66zu] = std::byte{ len };

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(world.name.data());
            for (u_char i = 0; i < len; ++i)
                data[68zu + i] = _1bit[i];

            u_int y = world.blocks.size() / 100, x = world.blocks.size() / y;
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
                pos += sizeof(short);

                if (block.water) data[pos - 1zu] |= std::byte{ 0x04 };
                if (block.glue)  data[pos - 1zu] |= std::byte{ 0x08 };
                if (block.fire)  data[pos - 1zu] |= std::byte{ 0x10 };
                // @todo add paint...
                switch (items[block.fg].type)
                {
                    case std::byte{ type::FOREGROUND }: 
                    case std::byte{ type::BACKGROUND }:
                        break;

                    case std::byte{ type::LOCK }: 
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
                    case std::byte{ type::MAIN_DOOR }: 
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        peer->pos.front() = (i % x) * 32;
                        peer->pos.back() = (i / x) * 32;
                        peer->rest_pos = peer->pos;
                        data.resize(data.size() + 8zu);

                        data[pos++] = std::byte{ 01 };
                        *reinterpret_cast<short*>(&data[pos]) = 4; pos += sizeof(short); // @note length of "EXIT"
                        *reinterpret_cast<std::array<std::byte, 4zu>*>(&data[pos]) = EXIT; pos += sizeof(std::array<std::byte, 4zu>);
                        data[pos++] = std::byte{ 00 }; // @note '\0'
                        break;
                    }
                    case std::byte{ type::SILKWORM }:
                    {
                        std::string dummy = "Lattish";
                        data[pos - 2] = std::byte{ 0x01 };
                        short len = static_cast<short>(dummy.length());
                        data.resize(data.size() + 50zu/*@todo*/ + len);

                        data[pos++] = std::byte{ 0x1f };
                        data[pos++] = std::byte{ 00 }; // @note died = 01

                        *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
                        for (const char& c : dummy) data[pos++] = static_cast<std::byte>(c);

                        short *uVar1 = reinterpret_cast<short*>(&data[pos]);

                        steady_clock::time_point dummy_1_day_age = steady_clock::now() + 24h;
                        uVar1[0] = (steady_clock::now() - dummy_1_day_age) / 1s; pos += sizeof(short);
                        uVar1[1] = (steady_clock::now() - dummy_1_day_age) / 24h; pos += sizeof(short);

                        uVar1[2] = 0x0000; pos += sizeof(short);
                        uVar1[3] = 0x0000; pos += sizeof(short); // @note silk ready to collect is 00 02

                        *reinterpret_cast<int*>(&data[pos]) = 2; pos += sizeof(int); 
                        data[pos++] = std::byte{ 01 };
                        uVar1 = reinterpret_cast<short*>(&data[pos]);

                        /* Hungry */
                        steady_clock::time_point dummy_hunger = steady_clock::now() + 13h;
                        uVar1[4] = (steady_clock::now() - dummy_hunger) / 1s; pos += sizeof(short);
                        uVar1[5] = 1; pos += sizeof(short);

                        /* Thirsty */
                        steady_clock::time_point dummy_thirst = steady_clock::now() + 13h;
                        uVar1[6] = (steady_clock::now() - dummy_thirst) / 1s; pos += sizeof(short);
                        uVar1[7] = 1; pos += sizeof(short);

                        /*                                      B G R A */
                        *reinterpret_cast<int*>(&data[pos]) = 0x496628ff; pos += sizeof(int);
                        uVar1 = reinterpret_cast<short*>(&data[pos]);

                        /* ill */
                        uVar1[8] = 0x0000; pos += sizeof(short);
                        uVar1[9] = 0; pos += sizeof(short);
                        break;
                    }
                    case std::byte { type::ENTRANCE }:
                    {
                        data[pos - 2zu] = (block._public) ? std::byte{ 0x90 } : std::byte{ 0x10 };
                        break;
                    }
                    case std::byte{ type::DOOR }:
                    case std::byte{ type::PORTAL }:
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
                    case std::byte{ type::SIGN }:
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
                    case std::byte{ type::SEED }:
                    {
                        data[pos - 2zu] = std::byte{ 0x11 };
                        data.resize(data.size() + 1zu + 5zu);

                        data[pos++] = std::byte{ 04 };
                        *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
                        data[pos++] = std::byte{ 03 }; // @note no clue...
                        break;
                    }
                    case std::byte{ type::PROVIDER }:
                    {
                        data[pos - 2zu] = std::byte{ 0x31 };
                        data.resize(data.size() + 5zu);

                        data[pos++] = std::byte{ 0x9 };
                        *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
                        break;
                    }
                    case std::byte{ type::WEATHER_MACHINE }: // @note there are no added bytes (I think)
                    {
                        if (block.toggled)
                            packet::create(*event.peer, false, 0, { "OnSetCurrentWeather", get_weather_id(block.fg) });
                        break;
                    }
                    case std::byte{ type::TOGGLEABLE_BLOCK }:
                    {
                        if (block.toggled) 
                        {
                            data[pos - 2zu] = std::byte{ 0x50 };
                        }
                        break;
                    }
                    case std::byte{ type::TOGGLEABLE_ANIMATED_BLOCK }:
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
                        data.resize(data.size() + 16zu); // @todo
                        break;
                }
                ++i;
            }
            pos += 12; // @note rgt has it as: bb 7f 06 00 00 00 00 00 5b 0d 0a 00

            *reinterpret_cast<int*>(&data[pos]) = world.ifloat_uid; pos += sizeof(int);
            *reinterpret_cast<int*>(&data[pos]) = world.ifloat_uid; pos += sizeof(int);
            for (const auto& ifloat : world.ifloats) 
            {
                data.resize(data.size() + 16zu);
                *reinterpret_cast<short*>(&data[pos]) = ifloat.second.id; pos += sizeof(short);
                *reinterpret_cast<float*>(&data[pos]) = ifloat.second.pos[0] * 32.0f; pos += sizeof(float);
                *reinterpret_cast<float*>(&data[pos]) = ifloat.second.pos[1] * 32.0f; pos += sizeof(float);
                *reinterpret_cast<short*>(&data[pos]) = ifloat.second.count; pos += sizeof(short);
                *reinterpret_cast<int*>(&data[pos]) = ifloat.first; pos += sizeof(int);
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

        on::EmoticonDataChanged(event);
        peer->netid = ++world.visitors;
        peer->prefix = (peer->role == MODERATOR) ? "#@" : (peer->role == DEVELOPER) ? "8@" : peer->prefix;

        peers(event, PEER_SAME_WORLD, [event, &peer, &world](ENetPeer& p) 
        {
            auto &_p = _peer[&p];

            constexpr std::string_view fmt = "spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n{}";
            packet::create(*event.peer, false, -1/* ff ff ff ff */, {
                "OnSpawn", 
                std::format(fmt, 
                    _p->netid, _p->user_id, static_cast<int>(_p->pos.front()), static_cast<int>(_p->pos.back()), _p->prefix, _p->ltoken[0], (_p->role >= MODERATOR) ? "1" : "0", (_p->role >= DEVELOPER) ? "1" : "0", 
                    ""
                ).c_str()
            });
            packet::create(p, false, -1/* ff ff ff ff */, {
                "OnSpawn", 
                std::format(fmt,
                    peer->netid, peer->user_id, static_cast<int>(peer->pos.front()), static_cast<int>(peer->pos.back()), peer->prefix, peer->ltoken[0], (peer->role >= MODERATOR) ? "1" : "0", (peer->role >= DEVELOPER) ? "1" : "0", 
                    (_p->user_id == peer->user_id) ? "type|local" : ""
                ).c_str()
            });
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
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) packet::create(*event.peer, false, 0, { "OnConsoleMessage", exc.what() });
        packet::create(*event.peer, false, 0, { "OnFailedToEnterWorld" });
    }
}