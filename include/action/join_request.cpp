#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "on/EmoticonDataChanged.hpp"
#include "tools/randomizer.hpp"
#include "join_request.hpp"

#include "tools/string_view.hpp"

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
        std::string big_name{world_name.empty() ? readch(std::string{header}, '|')[3] : world_name};
        if (not alpha(big_name) || big_name.empty()) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
        std::for_each(big_name.begin(), big_name.end(), [](char& c) { c = std::toupper(c); }); // @note start -> START
        std::unique_ptr<world> w = std::make_unique<world>(std::move(world().read(big_name)));
        if (w->name.empty())
        {
            const unsigned main_door = randomizer(2, 100 * 60 / 100 - 4);

            std::vector<block> blocks(100 * 60, block{0, 0});
            block *base_ptr = blocks.data();
            std::ranges::for_each(blocks, [&](block& b)
            {
                long long i = &b - base_ptr;
                if (i >= 3700)
                {
                    b.bg = 14; // cave background
                    if (i >= 3800 && i < 5000 /* (above) lava level */ && !randomizer(0, 38)) b.fg = 10/* rock */;
                    else if (i > 5000 && i < 5400 /* (above) bedrock level */ && randomizer(0, 8) < 3) b.fg = 4/* lava */;
                    else b.fg = (i >= 5400) ? 8 : 2/* dirt */;
                }
                if (i == 3600 + main_door) b.fg = 6; // main door
                else if (i == 3700 + main_door) b.fg = 8; // bedrock (below main door)
            });

            w->blocks = std::move(blocks);
            w->name = big_name; // init
        }
        {
            std::vector<std::byte> data(85 + w->name.length() + 5/*unknown*/ + (8 * w->blocks.size()) + 12/*initial drop*/, std::byte{ 00 });
            data[0] = std::byte{ 04 };
            data[4] = std::byte{ 04 };
            data[16] = std::byte{ 0x8 };
            unsigned char len = static_cast<unsigned char>(w->name.length());
            data[66] = std::byte{ len };
            for (unsigned char i = 0; i < len; ++i)
                *reinterpret_cast<char*>(&data[68 + i]) = w->name[i];
            unsigned y = w->blocks.size() / 100, x = w->blocks.size() / y;
            *reinterpret_cast<unsigned*>(&data[68 + len]) = x;
            *reinterpret_cast<unsigned*>(&data[72 + len]) = y;
            *reinterpret_cast<unsigned short*>(&data[76 + len]) = static_cast<unsigned short>(w->blocks.size());
            int pos = 85 + len;
            short i = 0;
            for (const block &block : w->blocks)
            {
                *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
                *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
                pos += sizeof(short); // @todo
                pos += sizeof(short); // @todo (water = 00 04)
                switch (items[block.fg].type)
                {
                    case std::byte{ type::FOREGROUND }: 
                    case std::byte{ type::BACKGROUND }:
                        break;

                    case std::byte{ type::LOCK }: 
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        std::size_t admins = std::ranges::count_if(w->admin, std::identity{});
                        data.resize(data.size() + 14zu + (admins * 4zu));
                        data[pos] = std::byte{ 03 }; pos += sizeof(std::byte);
                        data[pos] = std::byte{ 00 }; pos += sizeof(std::byte);
                        *reinterpret_cast<int*>(&data[pos]) = w->owner; pos += sizeof(int);
                        *reinterpret_cast<int*>(&data[pos]) = admins + 1; pos += sizeof(int);
                        *reinterpret_cast<int*>(&data[pos]) = -100; pos += sizeof(int);
                        /* @todo admin list */

                        break;
                    }
                    case std::byte{ type::MAIN_DOOR }: 
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        _peer[event.peer]->pos.front() = (i % x) * 32;
                        _peer[event.peer]->pos.back() = (i / x) * 32;
                        _peer[event.peer]->rest_pos = _peer[event.peer]->pos; // @note static repsawn position
                        data.resize(data.size() + 8zu);
                        data[pos] = std::byte{ 01 }; pos += sizeof(std::byte);
                        *reinterpret_cast<short*>(&data[pos]) = 4; pos += sizeof(short); // @note length of "EXIT"
                        *reinterpret_cast<std::array<std::byte, 4zu>*>(&data[pos]) = EXIT; pos += sizeof(std::array<std::byte, 4zu>);
                        data[pos] = std::byte{ 00 }; pos += sizeof(std::byte); // @note '\0'
                        
                        break;
                    }
                    case std::byte{ type::DOOR }:
                    {
                        data[pos - 2zu] = std::byte{ 01 };
                        std::span<const char> label = block.label;
                        short len{ static_cast<short>(label.size()) };
                        data.resize(data.size() + 4zu + len); // @note 01 {2} {} 0 0
                        data[pos] = std::byte{ 01 }; pos += sizeof(std::byte);
                        *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
                        for (const char& c : label) data[pos++] = static_cast<std::byte>(c);
                        data[pos] = std::byte{ 00 }; pos += sizeof(std::byte); // @note '\0'
                        break;
                    }
                    case std::byte{ type::SIGN }:
                    {
                        data[pos - 2zu] = std::byte{ 0x19 };
                        std::span<const char> label = block.label;
                        short len{ static_cast<short>(label.size()) };
                        data.resize(data.size() + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff
                        data[pos] = std::byte{ 02 }; pos += sizeof(std::byte);
                        *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
                        for (const char& c : label) data[pos++] = static_cast<std::byte>(c);
                        *reinterpret_cast<int*>(&data[pos]) = -1; pos += sizeof(int); // @note ff ff ff ff
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

        for (const auto& ifloat : w->ifloats)
        {
            std::vector<std::byte> compress = compress_state({
                .type = 0x0e, 
                .netid = -1,
                .peer_state = static_cast<int>(ifloat.uid),
                .count = static_cast<float>(ifloat.count),
                .id = ifloat.id, 
                .pos = {ifloat.pos[0] * 32, ifloat.pos[1] * 32}
            });
            send_data(*event.peer, compress);
        } // @note delete compress
        if (std::ranges::find(_peer[event.peer]->recent_worlds, w->name) == _peer[event.peer]->recent_worlds.end()) 
        {
            std::ranges::rotate(_peer[event.peer]->recent_worlds, _peer[event.peer]->recent_worlds.begin() + 1);
            _peer[event.peer]->recent_worlds.back() = w->name;
        }
        std::string& prefix = _peer[event.peer]->prefix;
        if (_peer[event.peer]->user_id == w->owner) prefix = "2";
        else if (std::ranges::find(w->admin, _peer[event.peer]->user_id) != w->admin.end()) prefix = "c";

        char& role = _peer[event.peer]->role;
        if (role == role::moderator) _peer[event.peer]->prefix = "8@";
        else if (role == role::developer) _peer[event.peer]->prefix = "6@";
        EmoticonDataChanged(event);
        _peer[event.peer]->post_enter.unlock();
        _peer[event.peer]->netid = ++w->visitors;
        peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
        {
            if (_peer[&p]->user_id != _peer[event.peer]->user_id) 
            {
                gt_packet(*event.peer, false, -1/* ff ff ff ff */, {
                    "OnSpawn", 
                    std::format("spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n",
                        _peer[&p]->netid, _peer[&p]->user_id, static_cast<int>(_peer[&p]->pos.front()), static_cast<int>(_peer[&p]->pos.back()), 
                        prefix, _peer[&p]->ltoken[0], (role >= role::moderator) ? "1" : "0", (role >= developer) ? "1" : "0"
                    ).c_str()
                });
                const char* enter_message = std::format("`5<`{}{}`` entered, `w{}`` others here>``", prefix, _peer[event.peer]->ltoken[0], w->visitors).c_str();
                gt_packet(p, false, 0, {
                    "OnConsoleMessage", 
                    enter_message
                });
                gt_packet(p, false, 0, {
                    "OnTalkBubble", 
                    _peer[event.peer]->netid, 
                    enter_message
                });
            }
        }); // @note delete enter_message
        /* @todo send this packet to everyone exept event.peer, and remove type|local */
        gt_packet(*event.peer, false, -1/* ff ff ff ff */, {
            "OnSpawn", 
            std::format("spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|`{}{}``\ncountry|us\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\ntype|local\n",
                _peer[event.peer]->netid, _peer[event.peer]->user_id, static_cast<int>(_peer[event.peer]->pos.front()), static_cast<int>(_peer[event.peer]->pos.back()), 
                prefix, _peer[event.peer]->ltoken[0], (role >= role::moderator) ? "1" : "0", (role >= developer) ? "1" : "0"
            ).c_str()
        });
        gt_packet(*event.peer, false, 0, {
            "OnConsoleMessage", 
            std::format("World `w{}`` entered.  There are `w{}`` other people here, `w{}`` online.", w->name, w->visitors - 1, peers(event).size()).c_str()
        });
        inventory_visuals(event);
        _peer[event.peer]->ready_exit = true;
        worlds.emplace(w->name, *w);
    }
    catch (const std::exception& exc)
    {
        if (exc.what() && *exc.what()) gt_packet(*event.peer, false, 0, { "OnConsoleMessage", exc.what() });
        gt_packet(*event.peer, false, 0, { "OnFailedToEnterWorld" });
    }
}