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
    if (auto it = std::ranges::find_if(this->slots, [s](const slot &found) { return found.id == s.id; }); it != this->slots.end()) 
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
    u_short &lvl = this->level.front();
    u_short &xp = this->level.back() += value; // @note factor the new xp amount

    const u_short xp_formula = 50 * (lvl * lvl + 2); // @note credits: https://www.growtopiagame.com/forums/member/553046-kasete
    const u_short level_up = std::min<u_short>(xp / xp_formula, 125 - lvl);
    
    lvl += level_up;
    xp -= level_up * xp_formula;
}

class peer_db {
private:
    sqlite3 *db;

    void sqlite3_bind(sqlite3_stmt* stmt, int index, int value) 
    {
        sqlite3_bind_int(stmt, index, value);
    }
    void sqlite3_bind(sqlite3_stmt* stmt, int index, const std::string& value) 
    {
        sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
    }
public:
    peer_db() {
        sqlite3_open("db/peers.db", &db);
        create_tables();
    }~peer_db() { sqlite3_close(db); }
    
    void create_tables() 
    {
        const char* sql = 
        "CREATE TABLE IF NOT EXISTS peers (_n TEXT PRIMARY KEY, role INTEGER, gems INTEGER, lvl INTEGER, xp INTEGER);"
        "CREATE TABLE IF NOT EXISTS slots (_n TEXT, i INTEGER, c INTEGER, FOREIGN KEY(_n) REFERENCES peers(_n));";

        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
    
    template<typename T>
    void execute(const char* sql, T binder) 
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            binder(stmt);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    
    template<typename T>
    void query(const char* sql, T &&fun, const std::string &name) 
    {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) 
        {
            sqlite3_bind(stmt, 1, name);
            
            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                fun(stmt);
            }
            sqlite3_finalize(stmt);
        }
    }
    
    void begin_transaction() 
    {
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    }
    
    void commit() 
    {
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    }
};

peer& peer::read(const std::string& name) 
{
    peer_db db;
    
    db.query("SELECT role, gems, lvl, xp FROM peers WHERE _n = ?", [this](sqlite3_stmt* stmt) 
    {
        this->role = static_cast<char>(sqlite3_column_int(stmt, 0));
        this->gems = sqlite3_column_int(stmt, 1);
        this->level[0] = static_cast<u_short>(sqlite3_column_int(stmt, 2));
        this->level[1] = static_cast<u_short>(sqlite3_column_int(stmt, 3));
    }, name);
    
    db.query("SELECT i, c FROM slots WHERE _n = ?", [this](sqlite3_stmt* stmt) 
    {
        this->slots.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1)
        );
    }, name);
    
    return *this;
}

peer::~peer() 
{
    if (ltoken[0].empty()) return;
    
    peer_db db;
    db.begin_transaction();
    
    db.execute("REPLACE INTO peers (_n, role, gems, lvl, xp) VALUES (?, ?, ?, ?, ?)", [this](sqlite3_stmt* stmt) 
    {
        sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, this->role);
        sqlite3_bind_int(stmt, 3, this->gems);
        sqlite3_bind_int(stmt, 4, this->level[0]);
        sqlite3_bind_int(stmt, 5, this->level[1]);
    });
    
    db.execute("DELETE FROM slots WHERE _n = ?", [this](auto stmt) {
        sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
    });
    
    for (const slot &s : this->slots) 
    {
        if ((s.id == 18 || s.id == 32) || s.count <= 0) continue;
        db.execute("INSERT INTO slots (_n, i, c) VALUES (?, ?, ?)", [this, &s](sqlite3_stmt* stmt) 
        {
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, s.id);
            sqlite3_bind_int(stmt, 3, s.count);
        });
    }
    db.commit();
}

std::unordered_map<ENetPeer*, std::shared_ptr<peer>> _peer;

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

void safe_disconnect_peers(int signal)
{
    for (ENetPeer &p : std::span(server->peers, server->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED) 
        {
            enet_peer_disconnect(&p, 0);
        }

    enet_deinitialize();
    exit(0);
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

std::vector<std::byte> compress_state(const state &s) 
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
}
