#include "pch.hpp"
#include "network/packet.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "tools/randomizer.hpp"
#include "commands/weather.hpp"
#include "join_request.hpp"

#include "tools/string_view.hpp"

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

void join_request(ENetEvent event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        if (not create_rt(event, 2, 900)) throw std::runtime_error("");

        auto &peer = _peer[event.peer];
        std::string big_name{world_name.empty() ? readch(std::string{header}, '|')[3] : world_name};

        if (not alpha(big_name) || big_name.empty()) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        
        world world(big_name);
        std::vector<std::string> buffs{};
        if (world.name.empty())
        {
            const unsigned main_door = randomizer(2, 100 * 60 / 100 - 4);
            std::vector<block> blocks(100 * 60, block{0, 0});
            
            for (auto &&[i, block] : blocks | std::views::enumerate)
            {
                if (i >= cord(0, 37))
                {
                    block.bg = 14; // cave background
                    if (i >= cord(0, 38) && i < cord(0, 50) /* (above) lava level */ && !randomizer(0, 38)) block.fg = 10 /* rock */;
                    else if (i > cord(0, 50) && i < cord(0, 54) /* (above) bedrock level */ && randomizer(0, 8) < 3) block.fg = 4 /* lava */;
                    else block.fg = (i >= cord(0, 54)) ? 8 : 2 /* dirt */;
                }
                if (i == cord(main_door, 36)) block.fg = 6; // main door
                else if (i == cord(main_door, 37)) block.fg = 8; // bedrock (below main door)
            }
            world.blocks = std::move(blocks);
            world.name = std::move(big_name);
        }
        {
            std::vector<std::byte> data(85 + world.name.length() + 5/*unknown*/ + (8 * world.blocks.size()) + 12/*initial drop*/, std::byte{ 00 });
            data[0zu] = std::byte{ 04 };
            data[4zu] = std::byte{ 04 }; // @note PACKET_SEND_MAP_DATA
            data[16zu] = std::byte{ 0x8 };
            unsigned char len = static_cast<unsigned char>(world.name.length());
            data[66zu] = std::byte{ len };

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(world.name.data());
            for (unsigned char i = 0; i < len; ++i)
                data[68zu + i] = _1bit[i];

            unsigned y = world.blocks.size() / 100, x = world.blocks.size() / y;
            *reinterpret_cast<unsigned*>(&data[68zu + len]) = x;
            *reinterpret_cast<unsigned*>(&data[72zu + len]) = y;
            *reinterpret_cast<unsigned short*>(&data[76zu + len]) = static_cast<unsigned short>(world.blocks.size());
            std::size_t pos = 85 + len;
            short i = 0;
            for (const block &block : world.blocks)
            {
                *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
                *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
                pos += sizeof(short); // @todo
                pos += sizeof(short); // @todo water = 00 04, glue = 00 08, both = 00 0c, fire = 00 10, paint (red) = 00 20, pattern repeats...
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
                    case std::byte{ type::DOOR }:
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        std::size_t len = block.label.length();
                        data.resize(data.size() + 4zu + len); // @note 01 {2} {} 0 0

                        data[pos++] = std::byte{ 01 };

                        *reinterpret_cast<short*>(&data[pos]) = static_cast<short>(len); pos += sizeof(short);
                        for (const char& c : block.label) data[pos++] = static_cast<std::byte>(c);
                        data[pos++] = std::byte{ 00 }; // @note '\0'
                        break;
                    }
                    case std::byte{ type::SIGN }:
                    {
                        data[pos - 2zu] = std::byte{ 0x19 };
                        std::size_t len = block.label.length();
                        data.resize(data.size() + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff

                        data[pos++] = std::byte{ 02 };

                        *reinterpret_cast<short*>(&data[pos]) = static_cast<short>(len); pos += sizeof(short);
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
                            gt_packet(*event.peer, false, 0, { "OnSetCurrentWeather", get_weather_id(block.fg) });
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
            enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
        } // @note delete data

        for (const auto& ifloat : world.ifloats)
        {
            std::vector<std::byte> compress = compress_state({
                .type = 0x0e, 
                .netid = -1,
                .peer_state = static_cast<int>(ifloat.uid),
                .count = static_cast<float>(ifloat.count),
                .id = ifloat.id, 
                .pos = {ifloat.pos[0] * 32, ifloat.pos[1] * 32}
            });
            send_data(*event.peer, std::move(compress));
        } // @note delete compress
        if (std::ranges::find(peer->recent_worlds, world.name) == peer->recent_worlds.end()) 
        {
            std::ranges::rotate(peer->recent_worlds, peer->recent_worlds.begin() + 1);
            peer->recent_worlds.back() = world.name;
        }
        if (peer->user_id == world.owner) peer->prefix = "2";
        else if (std::ranges::find(world.admin, peer->user_id) != world.admin.end()) peer->prefix = "c";

        char& role = peer->role;
        if (role == role::MODERATOR) peer->prefix = "8@";
        else if (role == role::DEVELOPER) peer->prefix = "6@";
        EmoticonDataChanged(event);
        peer->netid = ++world.visitors;
        peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
        {
            if (_peer[&p]->user_id != peer->user_id) 
            {
                gt_packet(*event.peer, false, -1/* ff ff ff ff */, {
                    "OnSpawn", 
                    std::format("spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n",
                        _peer[&p]->netid, _peer[&p]->user_id, static_cast<int>(_peer[&p]->pos.front()), static_cast<int>(_peer[&p]->pos.back()), 
                        peer->prefix, _peer[&p]->ltoken[0], (role >= role::MODERATOR) ? "1" : "0", (role >= role::DEVELOPER) ? "1" : "0"
                    ).c_str()
                });
                std::string enter_message{ std::format("`5<`{}{}`` entered, `w{}`` others here>``", peer->prefix, peer->ltoken[0], world.visitors) };
                gt_packet(p, false, 0, {
                    "OnConsoleMessage", 
                    enter_message.c_str()
                });
                gt_packet(p, false, 0, {
                    "OnTalkBubble", 
                    peer->netid, 
                    enter_message.c_str()
                });
            }
        }); // @note delete enter_message
        /* @todo send this packet to everyone exept event.peer, and remove type|local */
        gt_packet(*event.peer, false, -1/* ff ff ff ff */, {
            "OnSpawn", 
            std::format("spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\ntype|local\n",
                peer->netid, peer->user_id, static_cast<int>(peer->pos.front()), static_cast<int>(peer->pos.back()), 
                peer->prefix, peer->ltoken[0], (role >= role::MODERATOR) ? "1" : "0", (role >= role::DEVELOPER) ? "1" : "0"
            ).c_str()
        });
        auto section = [](const auto& range) 
        {
            if (range.empty()) return std::string{};
            
            std::string list{};
            for (const auto& buff : range) 
                list.append(std::format("{}``,", buff));
            list.pop_back();

            return std::format(" `0[``{}`0]``", list);
        };
        gt_packet(*event.peer, false, 0, {
            "OnConsoleMessage", 
            std::format(
                "World `w{}{}`` entered.  There are `w{}`` other people here, `w{}`` online.", 
                world.name, section(buffs), world.visitors - 1, peers(event).size()
            ).c_str()
        });
        inventory_visuals(event);
        peer->ready_exit = true;
        worlds.emplace(world.name, world); // @todo possible race-condition..
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) gt_packet(*event.peer, false, 0, { "OnConsoleMessage", exc.what() });
        gt_packet(*event.peer, false, 0, { "OnFailedToEnterWorld" });
    }
}