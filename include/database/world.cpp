#include "pch.hpp"
#include "tools/ransuu.hpp"
#include "on/ConsoleMessage.hpp"

#include "world.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

world::world(const std::string& name) 
{
    this->name = name;
}

std::vector<world> worlds;

void send_action(ENetPeer& p, const std::string& action, const std::string& str) 
{
    const std::string &fmt_action = std::format("action|{}\n", action);
    std::vector<u_char> data(sizeof(int) + fmt_action.length() + str.length(), 0x00);
    
    data[0] = 3; // @note NET_MESSAGE_GAME_MESSAGE
    {
        const u_char *i8 = reinterpret_cast<const u_char*>(fmt_action.c_str());
        for (std::size_t i = 0ull; i < fmt_action.length(); ++i)
            data[sizeof(int) + i] = i8[i];
    }
    if (!str.empty())
    {
        const u_char *i8 = reinterpret_cast<const u_char*>(str.c_str());
        for (std::size_t i = 0ull; i < str.length(); ++i)
            data[sizeof(int) + fmt_action.length() + i] = i8[i];
    }
    
    enet_peer_send(&p, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}

void send_data(ENetPeer &peer, const std::vector<u_char> &&data)
{
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    if (packet == nullptr || packet->dataLength < sizeof(::state)) return;

    enet_peer_send(&peer, 1, packet);
}

void state_visuals(ENetPeer &peer, state &&state) 
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);

    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) 
    {
        send_data(p, compress_state(state));
    });
}

void tile_apply_damage(ENetEvent& event, state state, block &block, u_int value)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    (block.fg == 0) ? ++block.hits[1] : ++block.hits[0];
    state.type = (value << 24) | 0x000008; // @note 0x{}000008
    state.id = 6; // @note idk exactly
    state.netid = pPeer->netid;
	state_visuals(*event.peer, std::move(state));
}

u_short modify_item_inventory(ENetEvent& event, ::slot slot)
{   
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::state state{.id = slot.id};
    if (slot.count < 0) state.type = (slot.count*-1 << 16) | 0x000d; // @noote 0x00{}000d
    else                state.type = (slot.count    << 24) | 0x000d; // @noote 0x{}00000d
    state_visuals(*event.peer, std::move(state));

    return pPeer->emplace(::slot(slot.id, slot.count));
}

void item_change_object(ENetEvent& event, ::state state) 
{
    state.type = 0x0e; // @note PACKET_ITEM_CHANGE_OBJECT

    state_visuals(*event.peer, std::move(state));
}

void merge_object(ENetEvent& event, ::slot slot, const ::pos& pos, ::world &world)
{
    auto object = std::ranges::find_if(world.objects, [&](const ::object &object) {
        return object.id == slot.id && (object.pos.by_32(true) == pos.by_32(true));
    });
    /* @todo avoid surpassing 200 and call add_object() for the remaining amount */ // @note future self reference peer::emplace()...
    object->count += slot.count;

    item_change_object(event, ::state{
        .netid = (int)0xfffffffd,
        .uid   = (int)object->uid,
        .count = static_cast<float>(object->count),
        .id    = object->id,
        .pos   = object->pos
    });
}

void remove_object(ENetEvent& event, signed uid)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    item_change_object(event, ::state{
        .netid = pPeer->netid,
        .uid   = (int)0xffffffff,
        .id    = uid
    });
}

int add_object(ENetEvent& event, ::slot slot, const ::pos& pos, ::world &world)
{
    /* @todo got a little messy */
    auto object = std::ranges::find_if(world.objects, [&](const ::object &object) {
        return object.id == slot.id && (object.pos.by_32(true) == pos.by_32(true));
    });
    if (object != world.objects.end() && object->count < 200/*@todo*/) 
    {
        merge_object(event, slot, pos, world);
        return object->uid;
    }
    ::object it = world.objects.emplace_back(::object(slot.id, slot.count, pos, ++world.last_object_uid)); // @note a iterator ahead of time

    item_change_object(event, ::state{
        .netid = (int)0xffffffff,
        .uid   = (int)it.uid,
        .count = static_cast<float>(slot.count),
        .id    = it.id,
        .pos   = pos
    });
    return it.uid;
}

void add_drop(ENetEvent &event, ::slot im, ::pos pos, ::world &world) // @todo
{
    ransuu ransuu;
    add_object(event, im, ::pos{
        pos.x + ransuu[{0, 16}],
        pos.y + ransuu[{0, 16}]
    }, world);
}

void send_tile_update(ENetEvent &event, state state, block &block, world &world) 
{
    state.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    state.peer_state = peer_state::S_EXTENDED;
    std::vector<u_char> data = compress_state(state);

    short pos = sizeof(::state); // @note start after state bytes (as every packet has)
    data.resize(pos + 8ull); // @note {2} {2} 00 00 00 00
    
    *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
    *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
    pos += sizeof(short);
    
    data[pos++] = block.state[2] ;
    data[pos++] = block.state[3];
    auto item = std::ranges::find(items, block.fg, &::item::id);
    switch (item->type)
    {
        case type::LOCK:
        {
            if (!is_tile_lock(block.fg)) world.is_public = (block.state[2] & S_PUBLIC); // @note check if world lock has S_PUBLIC flag, i will change this later

            int access = std::ranges::count_if(world.access, std::identity{});
            data.resize(data.size() + 1ull + 1ull + 4ull + 4ull + 4ull + (access * 4));

            data[pos++] = 0x03;
            data[pos++] = world.lock_state;
            *reinterpret_cast<int*>(&data[pos]) = world.owner; pos += sizeof(int);
            *reinterpret_cast<int*>(&data[pos]) = access; pos += sizeof(int);
            /* @todo access list */
            break;
        }
        case type::DOOR:
        case type::PORTAL:
        {
            short len = block.label.length();
            data.resize(pos + 1ull + 2ull + len + 1ull); // @note 01 {2} {} 0 0

            data[pos++] = 0x01;
            
            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            data[pos++] = '\0';
            break;
        }
        case type::SIGN:
        {
            short len = block.label.length();
            data.resize(pos + 1ull + 2ull + len + 4ull); // @note 02 {2} {} ff ff ff ff

            data[pos++] = 0x02;

            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            *reinterpret_cast<int*>(&data[pos]) = 0xffffffff; pos += sizeof(int);
            break;
        }
        case type::SEED:
        {
            data.resize(pos + 1ull + 5ull);

            data[pos++] = 0x04;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            data[pos++] = 0x03; // @note fruit on tree
            break;
        }
        case type::PROVIDER:
        {
            data.resize(pos + 5ull);

            data[pos++] = 0x09;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            break;
        }
        case DISPLAY_BLOCK:
        {
            data.resize(pos + 1ull + 4ull);
            auto display = std::ranges::find(world.displays, state.punch, &::display::pos);

            data[pos++] = 0x17;
            *reinterpret_cast<int*>(&data[pos]) = display->id; pos += sizeof(int);
            break;
        }
    }

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, std::move(data));
    });
}

void send_particle_effect(ENetEvent &event, const ::pos& pos, ::pos speed)
{
    state_visuals(*event.peer, ::state{
        .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
        .pos = pos,
        .speed = speed
    });
}

void remove_fire(ENetEvent &event, state state, block &block, world& world)
{
    send_particle_effect(event, {0x00, 0x95}, state.punch.by_32());

    block.state[3] &= ~S_FIRE;
    send_tile_update(event, state, block, world);

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (++pPeer->fires_removed % 100 == 0) 
    {
        on::ConsoleMessage(event.peer, "`oI'm so good at fighting fires, I rescused this `2Highly Combustible Box``!");
        modify_item_inventory(event, {3090/*Combustible Box*/, 1});
    }
    pPeer->add_xp(event, 1);
}

void generate_world(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    for (std::size_t i = 0ull; i < blocks.size(); ++i)
    {
        ::block &block = blocks[i];
        if (i >= cord(0, 37))
        {
            block.bg = 14; // @note cave background
            if (i >= cord(0, 38) && i < cord(0, 50) /* (above) lava level */ && ransuu[{0, 38}] <= 1) block.fg = 10; // rock
            else if (i > cord(0, 50) && i < cord(0, 54) /* (above) bedrock level */ && ransuu[{0, 8}] < 3) block.fg = 4; // lava
            else block.fg = (i >= cord(0, 54)) ? 8 : 2;
        }
        if (i == cord(main_door, 36)) block.fg = 6, block.label = "EXIT"; // @note main door
        else if (i == cord(main_door, 37)) block.fg = 8; // @note bedrock (below main door)
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}

bool door_mover(world &world, const ::pos &pos)
{
    std::vector<block> &blocks = world.blocks;

    if (blocks[cord(pos.x, pos.y)].fg != 0 ||
        blocks[cord(pos.x, (pos.y + 1))].fg != 0) return false;

    for (std::size_t i = 0ull; i < blocks.size(); ++i)
    {
        if (blocks[i].fg == 6)
        {
            blocks[i].fg = 0; // remove main door
            blocks[cord(i % 100, (i / 100 + 1))].fg = 0; // remove bedrock below
            break;
        }
    }
    blocks[cord(pos.x, pos.y)].fg = 6;
    blocks[cord(pos.x, (pos.y + 1))].fg = 8;
    return true;
}

void blast::thermonuclear(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    for (std::size_t i = 0ull; i < blocks.size(); ++i)
    {
        blocks[i].fg = (i >= cord(0, 54)) ? 8 : 0;

        if (i == cord(main_door, 36)) blocks[i].fg = 6;
        else if (i == cord(main_door, 37)) blocks[i].fg = 8;
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}
