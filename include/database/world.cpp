#include "pch.hpp"
#include "tools/ransuu.hpp"
#include "on/ConsoleMessage.hpp"
#include "database.hpp"

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
        for (std::size_t i = 0zu; i < fmt_action.length(); ++i)
            data[sizeof(int) + i] = i8[i];
    }
    if (!str.empty())
    {
        const u_char *i8 = reinterpret_cast<const u_char*>(str.c_str());
        for (std::size_t i = 0zu; i < str.length(); ++i)
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

int item_change_object(ENetEvent& event, ::slot slot, const ::pos& pos, signed uid) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::state state{.type = 0x0e}; // @note PACKET_ITEM_CHANGE_OBJECT

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return -1;

    auto object = std::ranges::find_if(world->objects, [&](const ::object &object) {
        return uid == 0 && object.id == slot.id && (object.pos.by_32(true) == pos.by_32(true));
    });

    if (object != world->objects.end()) // @note merge drop
    {
        object->count += slot.count;
        state.netid = 0xfffffffd;
        state.uid = object->uid;
        state.count = static_cast<float>(object->count);
        state.id =  object->id;
        state.pos = object->pos;
    }
    else if (slot.count == 0 || slot.id == 0) // @note remove drop
    {
        state.netid = pPeer->netid;
        state.uid = 0xffffffff;
        state.id = uid;
    }
    else // @note add new drop
    {
        auto it = world->objects.emplace_back(::object(slot.id, slot.count, pos, ++world->last_object_uid)); // @note a iterator ahead of time
        state.netid = 0xffffffff;
        state.uid = it.uid;
        state.count = static_cast<float>(slot.count);
        state.id = it.id;
        state.pos = pos;
    }
    state_visuals(*event.peer, std::move(state));
    return state.uid;
}

void add_drop(ENetEvent &event, ::slot im, ::pos pos)
{
    ransuu ransuu;
    item_change_object(event, {im.id, im.count},
    {
        pos.x + ransuu[{0, 16}],
        pos.y + ransuu[{0, 16}]
    });
}

void send_tile_update(ENetEvent &event, state state, block &block, world &world) 
{
    state.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    state.peer_state = peer_state::S_EXTENDED;
    std::vector<u_char> data = compress_state(state);

    short pos = sizeof(::state); // @note start after state bytes (as every packet has)
    data.resize(pos + 8zu); // @note {2} {2} 00 00 00 00
    
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

            std::size_t admins = std::ranges::count_if(world.admin, std::identity{});
            data.resize(data.size() + 1zu + 1zu + 4zu + 4zu + 4zu + (admins * 4zu));

            data[pos++] = 0x03;
            data[pos++] = world.lock_state;
            *reinterpret_cast<int*>(&data[pos]) = world.owner; pos += sizeof(int);
            *reinterpret_cast<int*>(&data[pos]) = admins; pos += sizeof(int);
            /* @todo admin list */
            break;
        }
        case type::DOOR:
        case type::PORTAL:
        {
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 1zu); // @note 01 {2} {} 0 0

            data[pos++] = 0x01;
            
            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            data[pos++] = '\0';
            break;
        }
        case type::SIGN:
        {
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff

            data[pos++] = 0x02;

            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            *reinterpret_cast<int*>(&data[pos]) = 0xffffffff; pos += sizeof(int);
            break;
        }
        case type::SEED:
        {
            data.resize(pos + 1zu + 5zu);

            data[pos++] = 0x04;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            data[pos++] = 0x03; // @note fruit on tree
            break;
        }
        case type::PROVIDER:
        {
            data.resize(pos + 5zu);

            data[pos++] = 0x09;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            break;
        }
        case DISPLAY_BLOCK:
        {
            data.resize(pos + 1zu + 4zu);
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

void remove_fire(ENetEvent &event, state state, block &block, world& world)
{
    state_visuals(*event.peer, ::state{
        .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
        .pos = state.punch.by_32(),
        .speed = ::pos{ 0x00000000, 0x95 }
    });

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
    
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
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

    for (std::size_t i = 0zu; i < blocks.size(); ++i)
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
    
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        blocks[i].fg = (i >= cord(0, 54)) ? 8 : 0;

        if (i == cord(main_door, 36)) blocks[i].fg = 6;
        else if (i == cord(main_door, 37)) blocks[i].fg = 8;
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}

void world::mysql_save()
{
    /* @note per-tile data -> single binary BLOB (leeendl's design).
     * layout: [u_char version][u_int count] then per-block:
     *   short fg | short bg | u_char state[4] | u_short label_len | char label[label_len]
     * labels live in the BLOB as a length-prefixed char array; empty tiles cost
     * just the 2-byte length (0). hits are intentionally NOT stored — they're
     * temporary (reset on countdown). world-level data (owner/flags) stays as columns. */
    constexpr u_char BLOB_VERSION = 1;
    std::vector<u_char> blob;
    blob.reserve(5 + this->blocks.size() * 10);

    auto push16 = [&](short v) {
        blob.push_back(static_cast<u_char>(v & 0xFF));
        blob.push_back(static_cast<u_char>((v >> 8) & 0xFF));
    };
    auto push32 = [&](u_int v) {
        blob.push_back(static_cast<u_char>(v & 0xFF));
        blob.push_back(static_cast<u_char>((v >> 8) & 0xFF));
        blob.push_back(static_cast<u_char>((v >> 16) & 0xFF));
        blob.push_back(static_cast<u_char>((v >> 24) & 0xFF));
    };

    blob.push_back(BLOB_VERSION);
    push32(static_cast<u_int>(this->blocks.size()));

    for (const ::block &b : this->blocks)
    {
        push16(b.fg);
        push16(b.bg);
        blob.push_back(b.state[0]);
        blob.push_back(b.state[1]);
        blob.push_back(b.state[2]);
        blob.push_back(b.state[3]);

        // label as length-prefixed char array (empty -> length 0, no bytes)
        u_short llen = static_cast<u_short>(b.label.size());
        push16(static_cast<short>(llen));
        for (char c : b.label) blob.push_back(static_cast<u_char>(c));
    }

    // serialize objects: id:count:x:y:uid;...
    std::string objects_str;
    for (const ::object &o : this->objects)
    {
        objects_str += std::format("{}:{}:{}:{}:{};",
            o.id, o.count, o.pos.x, o.pos.y, o.uid);
    }
    // sentinel: save last_object_uid
    objects_str += std::format("0:0:0:0:{};", this->last_object_uid);

    // serialize doors: dest:id:password:x:y;...
    std::string doors_str;
    for (const ::door &d : this->doors)
    {
        doors_str += std::format("{}:{}:{}:{}:{};",
            d.dest, d.id, d.password, d.pos.x, d.pos.y);
    }

    // serialize displays: id:x:y;...
    std::string displays_str;
    for (const ::display &d : this->displays)
    {
        displays_str += std::format("{}:{}:{};", d.id, d.pos.x, d.pos.y);
    }

    /* binary BLOB has null bytes -> must use a prepared statement, not string interpolation. */
    ::hStmt hStmt{
        "INSERT INTO world (name, owner, is_public, lock_state, minimum_entry_level, blocks, objects, doors, displays) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE owner=VALUES(owner), is_public=VALUES(is_public), lock_state=VALUES(lock_state), "
        "minimum_entry_level=VALUES(minimum_entry_level), blocks=VALUES(blocks), "
        "objects=VALUES(objects), doors=VALUES(doors), displays=VALUES(displays)"
    };

    int owner = this->owner;
    int is_public = this->is_public;
    int lock_state = this->lock_state;
    int min_level = this->minimum_entry_level;

    MYSQL_BIND params[9] = {
        make_bind_in(this->name),
        make_bind_in(owner),
        make_bind_in(is_public),
        make_bind_in(lock_state),
        make_bind_in(min_level),
        make_bind_in(blob),
        make_bind_in(objects_str),
        make_bind_in(doors_str),
        make_bind_in(displays_str)
    };
    mysql_stmt_bind_param(hStmt.pStmt, params);

    if (mysql_stmt_execute(hStmt.pStmt))
        fprintf(stderr, "[world::mysql_save] %s: %s\n", this->name.c_str(), mysql_stmt_error(hStmt.pStmt));
    else
        printf("[world::mysql_save] saved world '%s' (%zu blocks, %zu objects, %zu blob bytes)\n",
            this->name.c_str(), this->blocks.size(), this->objects.size(), blob.size());
}

bool world::mysql_load()
{
    auto escape = [](const std::string &s) -> std::string {
        char *buf = new char[s.size() * 2 + 1];
        mysql_real_escape_string(db, buf, s.c_str(), s.size());
        std::string result(buf);
        delete[] buf;
        return result;
    };

    std::string query = "SELECT owner, is_public, lock_state, minimum_entry_level, blocks, objects, doors, displays FROM world WHERE name = '"
        + escape(this->name) + "' LIMIT 1";

    if (mysql_query(db, query.c_str()))
    {
        fprintf(stderr, "[world::mysql_load] %s: %s\n", this->name.c_str(), mysql_error(db));
        return false;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false; // world not in DB
    }
    unsigned long *lengths = mysql_fetch_lengths(res); // @note BLOB has null bytes, can't use strlen

    this->owner = row[0] ? std::atoi(row[0]) : 0;
    this->is_public = row[1] ? std::atoi(row[1]) : 0;
    this->lock_state = row[2] ? (u_char)std::atoi(row[2]) : 0;
    this->minimum_entry_level = row[3] ? (u_char)std::atoi(row[3]) : 1;

    /* @note decode per-tile binary BLOB. layout:
     *   [u_char version][u_int count] then per-block:
     *   fg(2) bg(2) state[4] label_len(2) label[label_len]. */
    if (row[4] && lengths && lengths[4] >= 5)
    {
        this->blocks.clear();
        const u_char *buf = reinterpret_cast<const u_char*>(row[4]);
        unsigned long len = lengths[4];
        std::size_t p = 0;

        u_char version = buf[p++];
        auto read32 = [&]() -> u_int {
            u_int v = buf[p] | (buf[p+1] << 8) | (buf[p+2] << 16) | ((u_int)buf[p+3] << 24);
            p += 4; return v;
        };
        auto read16 = [&]() -> u_short {
            u_short v = (u_short)(buf[p] | (buf[p+1] << 8));
            p += 2; return v;
        };

        if (version == 1)
        {
            u_int count = read32();
            this->blocks.reserve(count);
            auto now = std::chrono::steady_clock::now();
            for (u_int i = 0; i < count && p + 8 <= len; ++i)
            {
                short fg = (short)read16();
                short bg = (short)read16();
                u_char s0 = buf[p++], s1 = buf[p++], s2 = buf[p++], s3 = buf[p++];

                u_short llen = read16();
                std::string label;
                if (llen > 0 && p + llen <= len)
                {
                    label.assign(reinterpret_cast<const char*>(buf + p), llen);
                    p += llen;
                }

                ::block b(fg, bg, now, label, s2, s3);
                b.state[0] = s0; b.state[1] = s1;
                this->blocks.emplace_back(std::move(b));
            }
        }
    }

    // parse objects: id:count:x:y:uid;...
    if (row[5])
    {
        this->objects.clear();
        u_int max_uid = 0;
        std::string objects_str(row[5]);
        auto parse_objects = readch(objects_str, ';');
        for (const auto &token : parse_objects)
        {
            auto parts = readch(token, ':');
            if (parts.size() >= 5)
            {
                u_short oid = (u_short)std::atoi(parts[0].c_str());
                if (oid == 0)
                {
                    // sentinel: 0:0:0:0:last_object_uid
                    this->last_object_uid = (u_int)std::atoi(parts[4].c_str());
                }
                else
                {
                    u_int uid = (u_int)std::atoi(parts[4].c_str());
                    if (uid > max_uid) max_uid = uid;
                    this->objects.emplace_back(
                        oid,
                        (u_short)std::atoi(parts[1].c_str()),
                        ::pos{static_cast<float>(std::atof(parts[2].c_str())), static_cast<float>(std::atof(parts[3].c_str()))},
                        uid
                    );
                }
            }
        }
        if (this->last_object_uid == 0 && max_uid > 0)
            this->last_object_uid = max_uid;
    }

    // parse doors: dest:id:password:x:y;...
    if (row[6])
    {
        this->doors.clear();
        std::string doors_str(row[6]);
        auto parse_doors = readch(doors_str, ';');
        for (const auto &token : parse_doors)
        {
            auto parts = readch(token, ':');
            if (parts.size() >= 5)
            {
                this->doors.emplace_back(
                    parts[0], parts[1], parts[2],
                    ::pos{static_cast<float>(std::atof(parts[3].c_str())), static_cast<float>(std::atof(parts[4].c_str()))}
                );
            }
        }
    }

    // parse displays: id:x:y;...
    if (row[7])
    {
        this->displays.clear();
        std::string displays_str(row[7]);
        auto parse_displays = readch(displays_str, ';');
        for (const auto &token : parse_displays)
        {
            auto parts = readch(token, ':');
            if (parts.size() >= 3)
            {
                this->displays.emplace_back(
                    (u_int)std::atoi(parts[0].c_str()),
                    ::pos{static_cast<float>(std::atof(parts[1].c_str())), static_cast<float>(std::atof(parts[2].c_str()))}
                );
            }
        }
    }

    mysql_free_result(res);
    printf("[world::mysql_load] loaded world '%s' (%zu blocks, %zu objects) from DB\n",
        this->name.c_str(), this->blocks.size(), this->objects.size());
    return true;
}
