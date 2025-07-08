#include "pch.hpp"
#include "items.hpp"
#include "peer.hpp"
#include "world.hpp"
#include "on/SetClothing.hpp"

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif

short peer::emplace(slot s) 
{
    if (auto it = std::ranges::find_if(this->slots, [&s](const slot &found) { return found.id == s.id; }); it != this->slots.end()) 
    {
        short excess = std::max(0, (it->count + s.count) - 200);
        it->count = std::min(it->count + s.count, 200);
        if (it->count == 0)
        {
            item &item = items[it->id];
            if (item.cloth_type != clothing::none) this->clothing[item.cloth_type] = 0;
        }
        return excess;
    }
    else this->slots.emplace_back(std::move(s)); // @note no such item in inventory, so we create a new entry.
    return 0;
}

void peer::add_xp(u_short value) 
{
    this->level.back() += value;

    u_short lvl = this->level.front();
    u_short xp_formula = 50 * (lvl * lvl + 2); // @note credits: https://www.growtopiagame.com/forums/member/553046-kasete
    
    u_short level_up = std::min<u_short>(this->level.back() / xp_formula, 125 - lvl);
    this->level.front() += level_up;
    this->level.back() -= level_up * xp_formula;
}

peer& peer::read(const std::string& name) 
{
    sqlite3 *db;
    if (sqlite3_open("db/peers.db", &db) != SQLITE_OK) return *this;
    struct DBCloser {
        sqlite3* db;
        ~DBCloser() { if (db) sqlite3_close(db); }
    } db_guard{db};
    {
        std::string create_tables =
            "CREATE TABLE IF NOT EXISTS peers ("
            "name TEXT PRIMARY KEY, role INTEGER, gems INTEGER, level0 INTEGER, level1 INTEGER);"

            "CREATE TABLE IF NOT EXISTS slots ("
            "name TEXT, id INTEGER, count INTEGER, FOREIGN KEY(name) REFERENCES peers(name));"

            "CREATE TABLE IF NOT EXISTS favs ("
            "name TEXT, id INTEGER, FOREIGN KEY(name) REFERENCES peers(name));";

        char *errmsg = nullptr;
        if (sqlite3_exec(db, create_tables.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) sqlite3_free(errmsg);
    } // @note delete create_tables
    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT role, gems, level0, level1 FROM peers WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                this->role = static_cast<char>(sqlite3_column_int(stmt, 0));
                this->gems = sqlite3_column_int(stmt, 1);
                this->level[0] = static_cast<u_short>(sqlite3_column_int(stmt, 2));
                this->level[1] = static_cast<u_short>(sqlite3_column_int(stmt, 3));
            }
        }
    }
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT id, count FROM slots WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                this->slots.emplace_back(slot(
                    sqlite3_column_int(stmt, 0),
                    sqlite3_column_int(stmt, 1)
                ));
            }
        }
    }
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT id FROM favs WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                this->fav.push_back(sqlite3_column_int(stmt, 0));
            }
        }
    }

    return *this;
}

peer::~peer() 
{
    sqlite3 *db;
    if (sqlite3_open("db/peers.db", &db) != SQLITE_OK) return;
    struct DBCloser {
        sqlite3* db;
        ~DBCloser() { if (db) sqlite3_close(db); }
    } db_guard{db};

    if (sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr) != SQLITE_OK) return;

    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "REPLACE INTO peers (name, role, gems, level0, level1) VALUES (?, ?, ?, ?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, this->role);
            sqlite3_bind_int(stmt, 3, this->gems);
            sqlite3_bind_int(stmt, 4, this->level[0]);
            sqlite3_bind_int(stmt, 5, this->level[1]);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "DELETE FROM slots WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) // @todo
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "INSERT INTO slots (name, id, count) VALUES (?, ?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            for (const slot &slot : this->slots) 
            {
                if ((slot.id == 18 || slot.id == 32) || slot.count <= 0) continue;
                sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, slot.id);
                sqlite3_bind_int(stmt, 3, slot.count);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }
    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "DELETE FROM favs WHERE name = ?;", -1, &stmt, nullptr) == SQLITE_OK) // @todo
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
    }
    {
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, "INSERT INTO favs (name, id) VALUES (?, ?);", -1, &stmt, nullptr) == SQLITE_OK) 
        {
            struct StmtFinalizer {
                sqlite3_stmt* stmt;
                ~StmtFinalizer() { sqlite3_finalize(stmt); }
            } stmt_guard{stmt};
            for (short &fav : this->fav) 
            {
                sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, fav);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::unordered_map<ENetPeer*, std::shared_ptr<peer>> _peer;

bool create_rt(ENetEvent &event, std::size_t pos, int length) 
{
    steady_clock::time_point &rt = _peer[event.peer]->rate_limit[pos];
    steady_clock::time_point now = steady_clock::now();

    if ((now - rt) <= std::chrono::milliseconds(length))
        return false;

    rt = now;
    return true;
}

ENetHost *server;

std::vector<ENetPeer*> peers(ENetEvent event, peer_condition condition, std::function<void(ENetPeer&)> fun)
{
    std::vector<ENetPeer*> _peers{};

    _peers.reserve(server->peerCount);

    auto &recent_worlds = _peer[event.peer]->recent_worlds;

    for (ENetPeer &peer : std::span(server->peers, server->peerCount))
        if (peer.state == ENET_PEER_STATE_CONNECTED) 
        {
            if (condition == peer_condition::PEER_SAME_WORLD)
            {
                if ((_peer[&peer]->recent_worlds.empty() && recent_worlds.empty()) || 
                    (_peer[&peer]->recent_worlds.back() != recent_worlds.back())) continue;
            }
            fun(peer);
            _peers.push_back(&peer);
        }

    return _peers;
}

state get_state(const std::vector<std::byte> &&packet) 
{
    const int *_4bit = reinterpret_cast<const int*>(packet.data());
    const float *_4bit_f = reinterpret_cast<const float*>(packet.data());
    return state{
        .type = _4bit[0],
        .netid = _4bit[1],
        .uid = _4bit[2],
        .peer_state = _4bit[3],
        .count = _4bit_f[4],
        .id = _4bit[5],
        .pos = {_4bit_f[6], _4bit_f[7]},
        .speed = {_4bit_f[8], _4bit_f[9]},

        .punch = {_4bit[11], _4bit[12]}
    };
}

std::vector<std::byte> compress_state(const state &&s) 
{
    std::vector<std::byte> data(56, std::byte{ 00 });
    int *_4bit = reinterpret_cast<int*>(data.data());
    float *_4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = s.type;
    _4bit[1] = s.netid;
    _4bit[2] = s.uid;
    _4bit[3] = s.peer_state;
    _4bit_f[4] = s.count;
    _4bit[5] = s.id;
    _4bit_f[6] = s.pos[0];
    _4bit_f[7] = s.pos[1];
    _4bit_f[8] = s.speed[0];
    _4bit_f[9] = s.speed[1];
    
    _4bit[11] = s.punch[0];
    _4bit[12] = s.punch[1];
    return data;
}

void inventory_visuals(ENetEvent &event)
{
    auto &peer = _peer[event.peer];
	std::size_t size = peer->slots.size();
    std::vector<std::byte> data(66zu + (size * sizeof(int)));
    
    data[0zu] = std::byte{ 04 };
    data[4zu] = std::byte{ 0x09 };
    *reinterpret_cast<int*>(&data[8zu]) = peer->netid;
    data[16zu] = std::byte{ 0x08 };
    *reinterpret_cast<int*>(&data[58zu]) = std::byteswap<int>(peer->slot_size);
    *reinterpret_cast<int*>(&data[62zu]) = std::byteswap<int>(size);
    int *slot_ptr = reinterpret_cast<int*>(&data[66zu]);
    for (const slot &slot : peer->slots) {
        *slot_ptr++ = (slot.id | (slot.count << 16)) & 0x00ffffff;
    }

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
    on::SetClothing(event); // @todo
}
