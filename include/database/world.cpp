#include "pch.hpp"
#include "items.hpp"
#include "peer.hpp"
#include "tools/ransuu.hpp"
#include "world.hpp"

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif

world::world(const std::string& name)
{
    sqlite3* db;
    if (sqlite3_open("db/worlds.db", &db) != SQLITE_OK) return;
    struct DBCloser {
        sqlite3* db;
        ~DBCloser() { if (db) sqlite3_close(db); }
    } db_guard{db};
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT owner FROM worlds WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                this->owner = sqlite3_column_int(stmt, 0);
                this->name = name;
            }
        }
    }
    {
        std::string create_table =
            "CREATE TABLE IF NOT EXISTS worlds ("
            "name TEXT PRIMARY KEY, owner INTEGER);"

            "CREATE TABLE IF NOT EXISTS blocks ("
            "world TEXT, fg INTEGER, bg INTEGER, public INTEGER, toggled INTEGER, tick INTEGER, label TEXT, "
            "FOREIGN KEY(world) REFERENCES worlds(name));"

            "CREATE TABLE IF NOT EXISTS doors ("
            "world TEXT, dest INTEGER, id INTEGER, password INTEGER, x REAL, y REAL, "
            "FOREIGN KEY(world) REFERENCES worlds(name));"

            "CREATE TABLE IF NOT EXISTS ifloats ("
            "world TEXT, uid INTEGER, id INTEGER, count INTEGER, x REAL, y REAL, "
            "PRIMARY KEY(world, uid), FOREIGN KEY(world) REFERENCES worlds(name));";

        char* errmsg = nullptr;
        if (sqlite3_exec(db, create_table.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) sqlite3_free(errmsg);
    } // @note delete create_table
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT fg, bg, public, toggled, tick, label FROM blocks WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                this->blocks.emplace_back(block(
                    sqlite3_column_int(stmt, 0),
                    sqlite3_column_int(stmt, 1),
                    sqlite3_column_int(stmt, 2),
                    sqlite3_column_int(stmt, 3),
                    steady_clock::time_point(std::chrono::seconds(sqlite3_column_int(stmt, 4))),
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))
                ));
            }
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT dest, id, password, x, y FROM doors WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                this->doors.emplace_back(door(
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
                    { sqlite3_column_int(stmt, 3), sqlite3_column_int(stmt, 4) }
                ));
            }
        }
    }
    {
        sqlite3_stmt* stmt;
        int max_uid = 0;
        if (sqlite3_prepare_v2(db, "SELECT uid, id, count, x, y FROM ifloats WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                int uid = sqlite3_column_int(stmt, 0);
                max_uid = std::max(max_uid, uid);
                this->ifloats.emplace(uid, ifloat(
                    sqlite3_column_int(stmt, 1),
                    sqlite3_column_int(stmt, 2),
                    { static_cast<float>(sqlite3_column_double(stmt, 3)),
                    static_cast<float>(sqlite3_column_double(stmt, 4)) }
                ));
            }
        }
        this->ifloat_uid = max_uid;
    }
}

world::~world()
{
    if (this->name.empty()) return;

    sqlite3* db;
    if (sqlite3_open("db/worlds.db", &db) != SQLITE_OK) return;
    struct DBCloser {
        sqlite3* db;
        ~DBCloser() { if (db) sqlite3_close(db); }
    } db_guard{db};

    sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr);

    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "REPLACE INTO worlds (name, owner) VALUES (?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, this->owner);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "DELETE FROM blocks WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) // @todo
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "INSERT INTO blocks (world, fg, bg, public, toggled, tick, label) VALUES (?, ?, ?, ?, ?, ?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            for (const block& b : this->blocks) {
                sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, b.fg);
                sqlite3_bind_int(stmt, 3, b.bg);
                sqlite3_bind_int(stmt, 4, b._public);
                sqlite3_bind_int(stmt, 5, b.toggled);
                sqlite3_bind_int(stmt, 6, static_cast<int>(duration_cast<std::chrono::seconds>(b.tick.time_since_epoch()).count()));
                sqlite3_bind_text(stmt, 7, b.label.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "DELETE FROM doors WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) // @todo
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "INSERT INTO doors (world, dest, id, password, x, y) VALUES (?, ?, ?, ?, ?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            for (door &door : this->doors) {
                sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, door.dest.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, door.id.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, door.password.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 5, door.pos[0]);
                sqlite3_bind_int(stmt, 6, door.pos[1]);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "DELETE FROM ifloats WHERE world = ?;", -1, &stmt, nullptr) == SQLITE_OK) // @todo
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "INSERT INTO ifloats (world, uid, id, count, x, y) VALUES (?, ?, ?, ?, ?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            for (const auto& ifloat : this->ifloats) {
                if (ifloat.second.id == 0 || ifloat.second.count == 0) continue;
                sqlite3_bind_text(stmt, 1, this->name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, ifloat.first);
                sqlite3_bind_int(stmt, 3, ifloat.second.id);
                sqlite3_bind_int(stmt, 4, ifloat.second.count);
                sqlite3_bind_double(stmt, 5, ifloat.second.pos[0]);
                sqlite3_bind_double(stmt, 6, ifloat.second.pos[1]);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::unordered_map<std::string, world> worlds;

void send_data(ENetPeer& peer, const std::vector<std::byte> &&data)
{
    std::size_t size = data.size();
    ENetPacket *packet = enet_packet_create(nullptr, size + 5zu, ENET_PACKET_FLAG_RELIABLE);

    *reinterpret_cast<int*>(&packet->data[0]) = 04;
    std::memcpy(packet->data + 4, data.data(), size);
    
    enet_peer_send(&peer, 1, packet);
}

void state_visuals(ENetEvent& event, state &&state) 
{
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, compress_state(std::move(state)));
    });
}

void tile_apply_damage(ENetEvent& event, state state, block &block)
{
    (block.fg == 0) ? ++block.hits.back() : ++block.hits.front();
    state.type = 0x08; // @note PACKET_TILE_APPLY_DAMAGE
    state.id = 6; // @note idk exactly
    state.netid = _peer[event.peer]->netid;
	state_visuals(event, std::move(state));
}

int item_change_object(ENetEvent& event, const std::array<short, 2zu>& im, const std::array<float, 2zu>& pos, signed uid) 
{
    std::vector<std::byte> compress{};
    state state{.type = 0x0e}; // @note PACKET_ITEM_CHANGE_OBJECT
    if (im[1] == 0 || im[0] == 0)
    {
        state.netid = _peer[event.peer]->netid;
        state.uid = -1;
        state.id = uid;
    }
    else
    {
        auto w = worlds.find(_peer[event.peer]->recent_worlds.back());
        if (w == worlds.end()) return -1;

        auto it = w->second.ifloats.emplace(++w->second.ifloat_uid, ifloat{im[0], im[1], pos}); // @note a iterator ahead of time
        state.netid = -1;
        state.uid = it.first->first;
        state.count = static_cast<float>(im[1]);
        state.id = it.first->second.id;
        state.pos = {it.first->second.pos[0] * 32, it.first->second.pos[1] * 32};
    }
    compress = compress_state(std::move(state));
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p)  
    {
        send_data(p, std::move(compress));
    });
    return state.uid;
}

void tile_update(ENetEvent &event, state state, block &block, world& w) 
{
    state.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    state.peer_state = 0x08;
    std::vector<std::byte> data = compress_state(std::move(state));

    short pos = 56;
    data.resize(pos + 8zu); // @note {2} {2} 00 00 00 00
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
        case std::byte{ type::ENTRANCE }:
        {
            data[pos - 2zu] = (block._public) ? std::byte{ 0x90 } : std::byte{ 0x10 };
            break;
        }
        case std::byte{ type::DOOR }:
        case std::byte{ type::PORTAL }:
        {
            data[pos - 2zu] = std::byte{ 01 };
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 1zu); // @note 01 {2} {} 0 0

            data[pos] = std::byte{ 01 }; pos += sizeof(std::byte);
            
            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char& c : block.label) data[pos++] = static_cast<std::byte>(c);
            pos += sizeof(std::byte); // @note '\0'
            break;
        }
        case std::byte{ type::SIGN }:
        {
            data[pos - 2zu] = std::byte{ 0x19 };
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff

            data[pos] = std::byte{ 02 }; pos += sizeof(std::byte);

            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char& c : block.label) data[pos++] = static_cast<std::byte>(c);
            *reinterpret_cast<int*>(&data[pos]) = -1; pos += sizeof(int); // @note ff ff ff ff
            break;
        }
    }

    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, std::move(data));
    });
}

void generate_world(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    for (auto &&[i, block] : blocks | std::views::enumerate)
    {
        if (i >= cord(0, 37))
        {
            block.bg = 14; // @note cave background
            if (i >= cord(0, 38) && i < cord(0, 50) /* (above) lava level */ && ransuu[{0, 38}] <= 1) block.fg = 10 /* rock */;
            else if (i > cord(0, 50) && i < cord(0, 54) /* (above) bedrock level */ && ransuu[{0, 8}] < 3) block.fg = 4 /* lava */;
            else block.fg = (i >= cord(0, 54)) ? 8 : 2;
        }
        if (i == cord(main_door, 36)) block.fg = 6; // @note main door
        else if (i == cord(main_door, 37)) block.fg = 8; // @note bedrock (below main door)
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}

bool door_mover(world &world, const std::array<int, 2ULL> &pos)
{
    if (world.blocks[cord(pos[0], pos[1])].fg != 0 ||
        world.blocks[cord(pos[0], (pos[1] + 1))].fg != 0) return false;

    for (std::size_t i = 0; i < world.blocks.size(); ++i)
    {
        block &block = world.blocks[i];
        if (block.fg == 6)
        {
            block.fg = 0; // @note remove main door
            world.blocks[cord(i % 100, (i / 100 + 1))].fg = 0; // @note remove bedrock (below main door)
            break;
        }
    }
    world.blocks[cord(pos[0], pos[1])].fg = 6;
    world.blocks[cord(pos[0], (pos[1] + 1))].fg = 8;
    return true;
}

void blast::thermonuclear(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    for (auto &&[i, block] : blocks | std::views::enumerate)
    {
        block.fg = (i >= cord(0, 54)) ? 8 : 0;

        if (i == cord(main_door, 36)) block.fg = 6;
        else if (i == cord(main_door, 37)) block.fg = 8;
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}
