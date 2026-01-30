#include "pch.hpp"
#include "items.hpp"
#include "peer.hpp"
#include "tools/ransuu.hpp"

#include "world.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

class world_db {
private:
    sqlite3* db;

    void sqlite3_bind(sqlite3_stmt* stmt, int i, int value)                 { sqlite3_bind_int(stmt, i, value); }
    void sqlite3_bind(sqlite3_stmt* stmt, int i, const std::string& value)  { sqlite3_bind_text(stmt, i, value.c_str(), -1, SQLITE_STATIC); }
public:
    world_db() {
        sqlite3_open("db/worlds.db", &db);
        create_tables();
    }~world_db() { sqlite3_close(db); }
    
    void create_tables() 
    {
        const char* sql = 
        "CREATE TABLE IF NOT EXISTS worlds (_n TEXT PRIMARY KEY, owner INTEGER, pub BOOLEAN);"

        "CREATE TABLE IF NOT EXISTS blocks ("
            "_n TEXT, _p INTEGER, fg INTEGER, bg INTEGER, tick INTEGER, l TEXT, s3 INTEGER, s4 INTEGER,"
            "PRIMARY KEY (_n, _p),"
            "FOREIGN KEY (_n) REFERENCES worlds(_n)"
        ");"

        "CREATE TABLE IF NOT EXISTS ifloats ("
            "_n TEXT, uid INTEGER, i INTEGER, c INTEGER, x REAL, y REAL,"
            "PRIMARY KEY (_n, uid),"
            "FOREIGN KEY (_n) REFERENCES worlds(_n)"
        ");";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
    
    template<typename F>
    void execute(const char* sql, F binder) 
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) 
        {
            binder(stmt);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    
    template<typename F>
    void query(const char* sql, F &&processor, const std::string &name) 
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) 
        {
            sqlite3_bind(stmt, 1, name);
            
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                processor(stmt);
            }
            sqlite3_finalize(stmt);
        }
    }
    
    void begin_transaction() { sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr); }
    void commit()            { sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr); }
};

world::world(const std::string& name) 
{
    world_db db;
    
    db.query("SELECT owner, pub FROM worlds WHERE _n = ?", [this, &name](sqlite3_stmt* stmt) 
    {
        this->owner =   sqlite3_column_int(stmt, 0);
        this->_public = sqlite3_column_int(stmt, 1);
        this->name = name;
    }, name);

    blocks.resize(6000);
    db.query("SELECT _p, fg, bg, tick, l, s3, s4 FROM blocks WHERE _n = ?", [this](sqlite3_stmt* stmt) 
    {
        int pos = sqlite3_column_int(stmt, 0);
        blocks[pos] = block(
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            steady_clock::time_point(std::chrono::seconds(sqlite3_column_int64(stmt, 3))),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_int(stmt, 6)
        );
    }, name);
     db.query("SELECT uid, i, c, x, y FROM ifloats WHERE _n = ?", [this](sqlite3_stmt* stmt) 
     {
        int uid = sqlite3_column_int(stmt, 0);
        ifloats.emplace(uid, ifloat(
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            ::pos{
                static_cast<float>(sqlite3_column_double(stmt, 3)), // @todo
                static_cast<float>(sqlite3_column_double(stmt, 4)) // @todo
            }
        ));
        ifloat_uid = std::max(ifloat_uid, uid);
    }, name);
}

world::~world() 
{
    if (this->name.empty()) return;
    
    world_db db;
    db.begin_transaction();
    
    db.execute("INSERT OR REPLACE INTO worlds (_n, owner, pub) VALUES (?, ?, ?)", [this](sqlite3_stmt* stmt) 
    {
            sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,  2, this->owner);
            sqlite3_bind_int(stmt,  3, this->_public);
    });
    
    db.execute("DELETE FROM blocks WHERE _n = ?", [this](auto stmt) {
        sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
    });
    for (std::size_t pos = 0; pos < this->blocks.size(); pos++) {
        const block &b = this->blocks[pos];
        db.execute("INSERT INTO blocks (_n, _p, fg, bg, tick, l, s3, s4) VALUES (?, ?, ?, ?, ?, ?, ?, ?)", [this, &b, &pos](sqlite3_stmt* stmt) 
        {
            int i = 1;
            sqlite3_bind_text(stmt,  i++, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,   i++, pos);
            sqlite3_bind_int(stmt,   i++, b.fg);
            sqlite3_bind_int(stmt,   i++, b.bg);
            sqlite3_bind_int64(stmt, i++, duration_cast<std::chrono::seconds>(b.tick.time_since_epoch()).count());
            sqlite3_bind_text(stmt,  i++, b.label.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,   i++, b.state3);
            sqlite3_bind_int(stmt,   i++, b.state4);
        });
    }

    db.execute("DELETE FROM ifloats WHERE _n = ?", [this](auto stmt) {
        sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
    });
    for (const auto &[uid, item] : this->ifloats) 
    {
        db.execute("INSERT INTO ifloats (_n, uid, i, c, x, y) VALUES (?, ?, ?, ?, ?, ?)", [&](sqlite3_stmt* stmt) 
        {
            int i = 1;
            sqlite3_bind_text(stmt,   i++, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,    i++, uid);
            sqlite3_bind_int(stmt,    i++, item.id);
            sqlite3_bind_int(stmt,    i++, item.count);
            sqlite3_bind_double(stmt, i++, item.pos.x*32);
            sqlite3_bind_double(stmt, i++, item.pos.y*32);
        });
    }
    
    db.commit();
}

std::unordered_map<std::string, world> worlds;

void send_data(ENetPeer& peer, const std::vector<u_char> &&data)
{
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    if (packet == nullptr || packet->dataLength < sizeof(::state)) return;

    enet_peer_send(&peer, 1, packet);
}

void state_visuals(ENetPeer& peer, state &&state) 
{
    ::peer *_p = static_cast<::peer*>(peer.data);

    peers(_p->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, compress_state(state));
    });
}

void tile_apply_damage(ENetEvent& event, state state, block &block)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    (block.fg == 0) ? ++block.hits.back() : ++block.hits.front();
    state.type = 0x08; // @note PACKET_TILE_APPLY_DAMAGE
    state.id = 6; // @note idk exactly
    state.netid = peer->netid;
	state_visuals(*event.peer, std::move(state));
}

short modify_item_inventory(ENetEvent& event, ::slot slot)
{   
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    ::state state{.id = slot.id};
    if (slot.count < 0) state.type = (slot.count*-1 << 16) | 0x000d; // @noote 0x00{}000d
    else                state.type = (slot.count    << 24) | 0x000d; // @noote 0x{}00000d
    state_visuals(*event.peer, std::move(state));

    return peer->emplace(::slot(slot.id, slot.count));
}

int item_change_object(ENetEvent& event, ::slot slot, const ::pos& pos, signed uid) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    ::state state{.type = 0x0e}; // @note PACKET_ITEM_CHANGE_OBJECT

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return -1;

    auto f = std::find_if(w->second.ifloats.begin(), w->second.ifloats.end(), [&](const std::pair<const int, ifloat>& entry) {
        return uid == 0 && entry.second.id == slot.id && entry.second.pos == pos;
    });
    if (f != w->second.ifloats.end()) // @note merge drop
    {
        f->second.count += slot.count;
        state.netid = -3; // @note fd ff ff ff
        state.uid = f->first;
        state.count = static_cast<float>(f->second.count);
        state.id = f->second.id;
        state.pos = f->second.pos;
    }
    else if (slot.count == 0 || slot.id == 0) // @note remove drop
    {
        state.netid = peer->netid;
        state.uid = -1;
        state.id = uid;
    }
    else // @note add new drop
    {
        auto it = w->second.ifloats.emplace(++w->second.ifloat_uid, ifloat{slot.id, slot.count, pos}); // @note a iterator ahead of time
        state.netid = -1;
        state.uid = it.first->first;
        state.count = static_cast<float>(slot.count);
        state.id = it.first->second.id;
        state.pos = it.first->second.pos;
    }
    state_visuals(*event.peer, std::move(state));
    return state.uid;
}

void add_drop(ENetEvent& event, ::slot im, ::pos pos)
{
    ransuu ransuu;
    item_change_object(event, {im.id, im.count},
    {
        pos.f_x() + ransuu.shosu({7, 50}, 0.01f), // @note (0.07 - 0.50)
        pos.f_y() + ransuu.shosu({7, 50}, 0.01f)  // @note (0.07 - 0.50)
    });
}

void tile_update(ENetEvent &event, state state, block &block, world& w) 
{
    state.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    state.peer_state = 0x08;
    std::vector<u_char> data = compress_state(state);

    short pos = sizeof(::state);
    data.resize(pos + 8zu); // @note {2} {2} 00 00 00 00
    
    *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
    *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
    pos += sizeof(short);
    
    data[pos++] = block.state3 ;
    data[pos++] = block.state4;
    switch (items[block.fg].type)
    {
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
            *reinterpret_cast<int*>(&data[pos]) = -1; pos += sizeof(int); // @note ff ff ff ff
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
    }

    ::peer *peer = static_cast<::peer*>(event.peer->data);
    peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, std::move(data));
    });
}

void remove_fire(ENetEvent &event, state state, block &block, world& w)
{
    state_visuals(*event.peer, ::state{
        .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
        .pos = state.punch,
        .speed = { 0x00000000, 0x95 }
    });

    block.state4 &= ~S_FIRE;
    tile_update(event, state, block, w);

    ::peer *peer = static_cast<::peer*>(event.peer->data);

    if (++peer->fires_removed % 100 == 0) 
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`oI'm so good at fighting fires, I rescused this `2Highly Combustible Box``!"
        });
        modify_item_inventory(event, {3090/*Combustible Box*/, 1});
    }

    peer->add_xp(event, 1);
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
