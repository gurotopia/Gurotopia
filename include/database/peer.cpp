#include "pch.hpp"
#include "items.hpp"
#include "world.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"

#include "peer.hpp"

using namespace std::chrono;

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

void peer::add_xp(ENetEvent &event, u_short value) 
{
    u_short &lvl = this->level.front();
    u_short &xp = this->level.back() += value; // @note factor the new xp amount

    for (; lvl < 125; )
    {
        u_short xp_formula = 50 * (lvl * lvl + 2); // @author https://www.growtopiagame.com/forums/member/553046-kasete
        if (xp < xp_formula) break;

        xp -= xp_formula;
        lvl++;

        if (lvl == 50) 
        {
            modify_item_inventory(event, ::slot{1400, 1}); // @note Mini Growtopian
            /* @todo based on account age give peer other items... */
        }
        if (lvl == 125) on::CountryState(event);

        /* @todo make particle effect smaller like growtopia */
        state_visuals(*event.peer, {
            .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
            .pos = this->pos, 
            .speed = {0, 0x2e}
        });
        packet::create(*event.peer, false, 0, {
            "OnTalkBubble", this->netid,
            std::format("`{}{}`` is now level {}!", this->prefix, this->ltoken[0], lvl).c_str()
        });
    }
}

class peer_db {
private:
    sqlite3 *db;

    void sqlite3_bind(sqlite3_stmt* stmt, int i, int value)                { sqlite3_bind_int(stmt, i, value); }
    void sqlite3_bind(sqlite3_stmt* stmt, int i, const std::string& value) { sqlite3_bind_text(stmt, i, value.c_str(), -1, SQLITE_STATIC); }
public:
    peer_db() {
        sqlite3_open("db/peers.db", &db);
        create_tables();
    }~peer_db() { sqlite3_close(db); }
    
    void create_tables() 
    {
        const char *sql = 
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
    
    void begin_transaction() { sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr); }
    
    void commit()            { sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr); }
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
        sqlite3_bind_int(stmt,  2, this->role);
        sqlite3_bind_int(stmt,  3, this->gems);
        sqlite3_bind_int(stmt,  4, this->level[0]);
        sqlite3_bind_int(stmt,  5, this->level[1]);
    });
    
    db.execute("DELETE FROM slots WHERE _n = ?", [this](auto stmt) {
        sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
    });
    
    for (const ::slot &slot : this->slots) 
    {
        if (slot.count <= 0) continue;
        db.execute("INSERT INTO slots (_n, i, c) VALUES (?, ?, ?)", [this, &slot](sqlite3_stmt* stmt) 
        {
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,  2, slot.id);
            sqlite3_bind_int(stmt,  3, slot.count);
        });
    }
    db.commit();
}

ENetHost *host;

std::vector<ENetPeer*> peers(const std::string &world, peer_condition condition, std::function<void(ENetPeer&)> fun)
{
    std::vector<ENetPeer*> _peers{};
    _peers.reserve(host->peerCount);

    for (ENetPeer &peer : std::span(host->peers, host->peerCount))
        if (peer.state == ENET_PEER_STATE_CONNECTED)
        {
            if (condition == peer_condition::PEER_SAME_WORLD)
            {
                ::peer *_p = static_cast<::peer*>(peer.data);
                if (_p->netid == 0 || (_p->recent_worlds.back() != world)) continue;
            }
            fun(peer);
            _peers.push_back(&peer);
        }

    return _peers;
}

void safe_disconnect_peers(int)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED)
            enet_peer_disconnect(&p, 0);

    enet_host_destroy(host);
    enet_deinitialize();
    puts("you killed gurotopia safely!");
}

state get_state(const std::vector<u_char> &&packet) 
{
    const int *_4bit = reinterpret_cast<const int*>(packet.data());
    const float *_4bit_f = reinterpret_cast<const float*>(packet.data());
    return state{
        .type = _4bit[1],
        .netid = _4bit[2],
        .uid = _4bit[3],
        .peer_state = _4bit[4],
        .count = _4bit_f[5],
        .id = _4bit[6],
        .pos = ::pos{_4bit_f[7], _4bit_f[8]},
        .speed = {_4bit_f[9], _4bit_f[10]},
        .idk = _4bit[11],
        .punch = ::pos{_4bit[12], _4bit[13]},
        .size = _4bit[14]
    };
}

std::vector<u_char> compress_state(const state &s) 
{
    std::vector<u_char> data(sizeof(::state) + 1, 0x00);
    int *_4bit = reinterpret_cast<int*>(data.data());
    float *_4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = s.packet_create;
    _4bit[1] = s.type;
    _4bit[2] = s.netid;
    _4bit[3] = s.uid;
    _4bit[4] = s.peer_state;
    _4bit_f[5] = s.count;
    _4bit[6] = s.id;
    ::pos f_pos = s.pos; // @todo
    _4bit_f[7] = f_pos.f_x();
    _4bit_f[8] = f_pos.f_y();
    _4bit_f[9] = s.speed[0];
    _4bit_f[10] = s.speed[1];
    _4bit[11] = s.idk;
    _4bit[12] = s.punch.x;
    _4bit[13] = s.punch.y;
    _4bit[14] = s.size;
    return data;
}

void send_inventory_state(ENetEvent &event)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    std::vector<u_char> data = compress_state(::state{
        .type = 0x09, // @note PACKET_SEND_INVENTORY_STATE
        .netid = peer->netid,
        .peer_state = 0x08
    });

    std::size_t size = peer->slots.size();
    data.resize(data.size() + 5zu + (size * sizeof(int)));

    int *_4bit = reinterpret_cast<int*>(&data[58zu]);

    *_4bit++ = std::byteswap<int>(peer->slot_size);
    *_4bit++ = std::byteswap<int>(size);
    for (const ::slot &slot : peer->slots)
        *_4bit++ = slot.id | (slot.count & 0xff) << 16;

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}
