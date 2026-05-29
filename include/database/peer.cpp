#include "pch.hpp"
#include "items.hpp"
#include "world.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"
#include "commands/punch.hpp"

#include "peer.hpp"

using namespace std::chrono;

short peer::emplace(slot s)
{
    constexpr short MAX_STACK = 200;

    auto it = std::ranges::find(this->slots, s.id, &::slot::id);

    // Item already exists in inventory
    if (it != this->slots.end())
    {
        short free_space = MAX_STACK - it->count;

        // Inventory already full
        if (free_space <= 0)
            return s.count;

        short added = std::min<short>(free_space, s.count);

        it->count += added;

        // Return leftover amount
        return s.count - added;
    }

    // New inventory slot
    short added = std::min<short>(MAX_STACK, s.count);

    this->slots.emplace_back(::slot(s.id, added));

    // Return leftover amount
    return s.count - added;
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
            .speed = ::pos{0, 0x2e}
        });
        packet::create(*event.peer, false, 0, {
            "OnTalkBubble", this->netid,
            std::format("`{}{}`` is now level {}!", this->prefix, this->ltoken[0], lvl).c_str()
        });
    }
}

void peer::update_effects()
{
    this->punch_effect = 0;
    bool allow_double_jump = false;
    for (float cloth : this->clothing)
    {
        const auto cloth_id = static_cast<u_int>(cloth);
        u_short punch_id = get_punch_id(cloth_id);
        if (punch_id != 0)
            this->punch_effect = punch_id;

        auto item = std::ranges::find(items, cloth_id, &::item::id);
        if (item != items.end() && item->cloth_type == clothing::back)
        {
            std::string lower_name = item->raw_name;
            std::ranges::transform(lower_name, lower_name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower_name.find("wing") != std::string::npos || lower_name.find("cape") != std::string::npos)
                allow_double_jump = true;
        }
    }

    if (allow_double_jump) this->state |= S_DOUBLE_JUMP;
    else this->state &= ~S_DOUBLE_JUMP;
}

class peer_db {
private:
    sqlite3 *db;

    void sqlite3_bind(sqlite3_stmt* stmt, int i, int value)                { sqlite3_bind_int(stmt, i, value); }
    void sqlite3_bind(sqlite3_stmt* stmt, int i, long long value)          { sqlite3_bind_int64(stmt, i, value); }
    void sqlite3_bind(sqlite3_stmt* stmt, int i, const std::string& value) { sqlite3_bind_text(stmt, i, value.c_str(), -1, SQLITE_STATIC); }
public:
    peer_db() {
        sqlite3_open("db/peers.db", &db);
        create_tables();
    }~peer_db() { sqlite3_close(db); }

    void create_tables() 
    {
        const char *sql = 
        "CREATE TABLE IF NOT EXISTS peers (_n TEXT PRIMARY KEY, role INTEGER, gems INTEGER, lvl INTEGER, xp INTEGER, name_changed_at INTEGER DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS slots (_n TEXT, i INTEGER, c INTEGER, FOREIGN KEY(_n) REFERENCES peers(_n));"
        "CREATE TABLE IF NOT EXISTS equip (_n TEXT, i INTEGER, FOREIGN KEY(_n) REFERENCES peers(_n));"
        "CREATE TABLE IF NOT EXISTS peer_aliases (alias TEXT PRIMARY KEY, current_name TEXT NOT NULL, FOREIGN KEY(current_name) REFERENCES peers(_n));";

        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        sqlite3_exec(db, "ALTER TABLE peers ADD COLUMN name_changed_at INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
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

    this->slots.clear();
    this->clothing.fill(0);
    this->name_changed_at = 0;

    const std::string resolved_name = peer::resolve_name(name);
    this->ltoken[0] = resolved_name;

    db.query("SELECT role, gems, lvl, xp, name_changed_at FROM peers WHERE _n = ?", [this](sqlite3_stmt* stmt) 
    {
        this->role = static_cast<u_char>(sqlite3_column_int(stmt, 0));
        this->gems = sqlite3_column_int(stmt, 1);
        this->level[0] = static_cast<u_short>(sqlite3_column_int(stmt, 2));
        this->level[1] = static_cast<u_short>(sqlite3_column_int(stmt, 3));
        this->name_changed_at = static_cast<long long>(sqlite3_column_int64(stmt, 4));
    }, resolved_name);

    db.query("SELECT i, c FROM slots WHERE _n = ?", [this](sqlite3_stmt* stmt) 
    {
        this->slots.emplace_back(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1)
        );
    }, resolved_name);

    u_short i{};
    db.query("SELECT i FROM equip WHERE _n = ?", [this, &i](sqlite3_stmt* stmt) 
    {
        this->clothing[i] = sqlite3_column_int(stmt, 0);
        ++i;
    }, resolved_name);

    this->update_effects();
    return *this;
}

std::string peer::resolve_name(const std::string& name)
{
    if (name.empty()) return name;

    peer_db db;
    std::string resolved = name;

    for (int i = 0; i < 10; ++i)
    {
        std::string next = resolved;
        db.query("SELECT current_name FROM peer_aliases WHERE alias = ? LIMIT 1", [&next](sqlite3_stmt* stmt)
        {
            if (const auto value = sqlite3_column_text(stmt, 0); value != nullptr)
                next = reinterpret_cast<const char*>(value);
        }, resolved);

        if (next == resolved) break;
        resolved = std::move(next);
    }

    return resolved;
}

bool peer::exists(const std::string& name)
{
    peer_db db;
    bool found = false;
    const std::string resolved_name = peer::resolve_name(name);

    db.query("SELECT 1 FROM peers WHERE _n = ? LIMIT 1", [&found](sqlite3_stmt*)
    {
        found = true;
    }, resolved_name);

    return found;
}

bool peer::rename(const std::string& old_name, const std::string& new_name)
{
    if (old_name.empty() || new_name.empty()) return false;

    const std::string resolved_old_name = peer::resolve_name(old_name);
    if (resolved_old_name == new_name) return false;

    peer_db db;
    bool updated = false;
    db.begin_transaction();

    db.execute("UPDATE peers SET _n = ? WHERE _n = ?", [&updated, &new_name, &resolved_old_name](sqlite3_stmt* stmt)
    {
        sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, resolved_old_name.c_str(), -1, SQLITE_STATIC);
        updated = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(sqlite3_db_handle(stmt)) > 0;
    });

    if (updated)
    {
        db.execute("UPDATE slots SET _n = ? WHERE _n = ?", [&new_name, &resolved_old_name](sqlite3_stmt* stmt)
        {
            sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, resolved_old_name.c_str(), -1, SQLITE_STATIC);
        });
        db.execute("UPDATE equip SET _n = ? WHERE _n = ?", [&new_name, &resolved_old_name](sqlite3_stmt* stmt)
        {
            sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, resolved_old_name.c_str(), -1, SQLITE_STATIC);
        });

        db.execute("INSERT OR REPLACE INTO peer_aliases (alias, current_name) VALUES (?, ?)", [&resolved_old_name, &new_name](sqlite3_stmt* stmt)
        {
            sqlite3_bind_text(stmt, 1, resolved_old_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, new_name.c_str(), -1, SQLITE_STATIC);
        });
        db.execute("UPDATE peer_aliases SET current_name = ? WHERE current_name = ?", [&new_name, &resolved_old_name](sqlite3_stmt* stmt)
        {
            sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, resolved_old_name.c_str(), -1, SQLITE_STATIC);
        });
        db.execute("DELETE FROM peer_aliases WHERE alias = ?", [&new_name](sqlite3_stmt* stmt)
        {
            sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_STATIC);
        });
    }

    db.commit();
    return updated;
}

peer::~peer() 
{
    if (this->ltoken[0].empty() && this->user_id == 0) return;
    
    peer_db db;
    db.begin_transaction();
    
    db.execute("INSERT OR REPLACE INTO peers (_n, role, gems, lvl, xp, name_changed_at) VALUES (?, ?, ?, ?, ?, ?)", [this](sqlite3_stmt* stmt) 
    {
        sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt,  2, this->role);
        sqlite3_bind_int(stmt,  3, this->gems);
        sqlite3_bind_int(stmt,  4, this->level[0]);
        sqlite3_bind_int(stmt,  5, this->level[1]);
        sqlite3_bind_int64(stmt, 6, this->name_changed_at);
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

    db.execute("DELETE FROM equip WHERE _n = ?", [this](auto stmt) {
        sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
    });
    for (const float &cloth : this->clothing)
    {
        db.execute("INSERT INTO equip (_n, i) VALUES (?, ?)", [this, &cloth](sqlite3_stmt* stmt) 
        {
            sqlite3_bind_text(stmt, 1, this->ltoken[0].c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt,  2, cloth);
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
                ::peer *pOthers = static_cast<::peer*>(peer.data);
                if (pOthers->netid == 0 || (pOthers->recent_worlds.back() != world)) continue;
            }
            fun(peer);
            _peers.push_back(&peer);
        }

    return _peers;
}

void safe_disconnect_peers(int code)
{
    puts("killing gurotopia...");
    if (!host)
    {
        puts("killed gurotopia safely!");
        return;
    }

    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED)
            enet_peer_disconnect(&p, 0);

    enet_host_flush(host);
    enet_host_destroy(host);
    host = nullptr;
    enet_deinitialize();
    puts("killed gurotopia safely!");
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
        .speed = ::pos{_4bit_f[9], _4bit_f[10]},
        .idk = _4bit[11],
        .punch = ::pos{_4bit[12], _4bit[13]},
        .size = _4bit[14]
    };
}

std::vector<u_char> compress_state(const state &state) 
{
    std::vector<u_char> data(sizeof(::state) + 1, 0x00);
    int *_4bit = reinterpret_cast<int*>(data.data());
    float *_4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = state.packet_create;
    _4bit[1] = state.type;
    _4bit[2] = state.netid;
    _4bit[3] = state.uid;
    _4bit[4] = state.peer_state;
    _4bit_f[5] = state.count;
    _4bit[6] = state.id;
    _4bit_f[7] = state.pos.x;
    _4bit_f[8] = state.pos.y;
    _4bit_f[9] = state.speed.x;
    _4bit_f[10] = state.speed.y;
    _4bit[11] = state.idk;
    _4bit[12] = state.punch.x;
    _4bit[13] = state.punch.y;
    _4bit[14] = state.size;
    return data;
}

void send_inventory_state(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<u_char> data = compress_state(::state{
        .type = 0x09, // @note PACKET_SEND_INVENTORY_STATE
        .netid = pPeer->netid,
        .peer_state = 0x08
    });

    std::size_t size = pPeer->slots.size();
    data.resize(data.size() + 5zu + (size * sizeof(int)));

    int *_4bit = reinterpret_cast<int*>(&data[58zu]);

    *_4bit++ = std::byteswap<int>(pPeer->slot_size);
    *_4bit++ = std::byteswap<int>(size);
    for (const ::slot &slot : pPeer->slots)
        *_4bit++ = slot.id | (slot.count & 0xff) << 16;

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}
