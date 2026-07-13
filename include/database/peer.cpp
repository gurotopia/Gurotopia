#include "pch.hpp"
#include "database.hpp"
#include "items.hpp"
#include "world.hpp"
#include "on/SetClothing.hpp"
#include "on/CountryState.hpp"
#include "commands/punch.hpp"
#include "tools/string.hpp"

#include "peer.hpp"

using namespace std::chrono;

bool peer::exists(const std::string& growid)
{
    ::hStmt hStmt{ "SELECT 1 FROM peer WHERE growid = ? LIMIT 1" };

    MYSQL_BIND param = make_bind_in(growid);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    return (!mysql_stmt_store_result(hStmt.pStmt) && mysql_stmt_num_rows(hStmt.pStmt) > 0);
}

template<typename T>
void peer::mysql_insert(const std::string& column, const T& value)
{
    ::hStmt hStmt{ std::format("INSERT INTO peer ({}) VALUES (?)", column).c_str() };

    MYSQL_BIND param = make_bind_in(value);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);
}
template void peer::mysql_insert<signed>(const std::string&, const signed&);
template void peer::mysql_insert<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_insert<float>(const std::string&, const float&);
template void peer::mysql_insert<std::string>(const std::string&, const std::string&);

template<typename T>
void peer::mysql_update(const std::string& column, const T& value)
{
    ::hStmt hStmt{ std::format("UPDATE peer SET {} = ? WHERE growid = ?", column).c_str() };

    MYSQL_BIND params[2] = {
        make_bind_in(value),       // SET
        make_bind_in(this->growid) // WHERE
    };
    mysql_stmt_bind_param(hStmt.pStmt, params);
    mysql_stmt_execute(hStmt.pStmt);
}
template void peer::mysql_update<signed>(const std::string&, const signed&);
template void peer::mysql_update<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_update<float>(const std::string&, const float&);
template void peer::mysql_update<std::string>(const std::string&, const std::string&);

template<typename T>
T peer::mysql_select(const std::string &column, const std::string &arg)
{
    T value{};
    ::hStmt hStmt{ std::format("SELECT {}({}) FROM peer WHERE growid = ? LIMIT 1", arg, column).c_str() };

    MYSQL_BIND param = make_bind_in(this->growid);
    mysql_stmt_bind_param(hStmt.pStmt, &param);

    unsigned long length = 0;
    MYSQL_BIND result = make_bind_out(value);
    result.length = &length;
    mysql_stmt_bind_result(hStmt.pStmt, &result);

    mysql_stmt_execute(hStmt.pStmt);
    mysql_stmt_fetch(hStmt.pStmt);

    // strings bind into a fixed 1024-byte buffer; trim to the real column
    // length so values compare byte-for-byte (PBKDF2 hashes were failing
    // verify because the buffer stayed padded with NULs).
    if constexpr (std::is_same_v<T, std::string>)
        value.resize(length);

    return value;
}
/* since we will only select during mysql_select_all */ // @note add templates here if use select outside of this file.

void peer::mysql_select_all()
{
    this->user_id    = this->mysql_select<signed>("uid");
    this->growid     = this->mysql_select<std::string>("growid");
    this->password   = this->mysql_select<std::string>("password");
    this->created_at = this->mysql_select<std::time_t>("created_at", "UNIX_TIMESTAMP");
}

u_short peer::emplace(::slot slot) 
{
    if (auto it = std::ranges::find(this->slots, slot.id, &::slot::id); it != this->slots.end()) 
    {
        u_short excess = std::max(0, (it->count + slot.count) - 200);
        it->count = std::min(it->count + slot.count, 200);
        if (it->count == 0)
        {
            auto item = std::ranges::find(items, it->id, &::item::id);
            if (item->cloth_type != clothing::none) this->clothing[item->cloth_type] = 0;
        }
        return excess;
    }
    else this->slots.emplace_back(std::move(slot)); // @note no such item in inventory, so we create a new entry.
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

        send_particle_effect(event, {0x00, 0x2e}, this->pos); // @todo make particle effect smaller like growtopia
        send_varlist(event.peer, { 
            "OnTalkBubble", 
            this->netid,
            std::format("`{}{}`` is now level {}!", this->prefix, this->growid, lvl) 
        });
    }
}

void peer::update_effects()
{
    this->punch_effect = 0;
    for (float cloth : this->clothing)
    {
        u_short punch_id = get_punch_id(static_cast<u_int>(cloth));
        if (punch_id != 0)
            this->punch_effect = punch_id;
    }
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

    peers("", peer_condition::PEER_ALL, [](ENetPeer &p) { enet_peer_disconnect(&p, 0); });
    enet_host_flush(host);
    enet_host_destroy(host);
    host = nullptr; // @todo clean this up better
    enet_deinitialize();

    puts("killed gurotopia safely!");
}

state get_state(const std::vector<u_char> &&packet) 
{
    const int     *i32   = reinterpret_cast<const int*>(packet.data());
    const u_int *u_i32 = reinterpret_cast<const u_int*>(packet.data());
    const float   *f_i32 = reinterpret_cast<const float*>(packet.data());

    return state{
        .type = i32[1],
        .netid = i32[2],
        .uid = i32[3],
        .peer_state = i32[4],
        .count = f_i32[5],
        .id = i32[6],
        .pos = ::pos{f_i32[7], f_i32[8]},
        .speed = ::pos{f_i32[9], f_i32[10]},
        .idk = i32[11],
        .punch = ::pos{i32[12], i32[13]},
        .size = u_i32[14]
    };
}

std::vector<u_char> compress_state(const state &state) 
{
    std::vector<u_char> data(sizeof(::state), 0x00);
    int   *i32   = reinterpret_cast<int*>(data.data());
    u_int *u_i32 = reinterpret_cast<u_int*>(data.data());
    float *f_i32 = reinterpret_cast<float*>(data.data());

    i32[0] = state.packet_create;
    i32[1] = state.type;
    i32[2] = state.netid;
    i32[3] = state.uid;
    i32[4] = state.peer_state;
    f_i32[5] = state.count;
    i32[6] = state.id;
    f_i32[7] = state.pos.x;
    f_i32[8] = state.pos.y;
    f_i32[9] = state.speed.x;
    f_i32[10] = state.speed.y;
    i32[11] = state.idk;
    i32[12] = state.punch.x;
    i32[13] = state.punch.y;
    u_i32[14] = state.size;
    return data;
}

void send_inventory_state(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<u_char> data = compress_state(::state{
        .type = 0x09, // @note PACKET_SEND_INVENTORY_STATE
        .netid = pPeer->netid,
        .peer_state = peer_state::S_EXTENDED
    });

    std::size_t size = pPeer->slots.size();
    data.resize(data.size() + 5zu + (size * sizeof(int)));

    int *i32 = reinterpret_cast<int*>(&data[58zu]);

    *i32++ = std::byteswap<int>(pPeer->slot_size);
    *i32++ = std::byteswap<int>(size);
    for (const ::slot &slot : pPeer->slots)
        *i32++ = slot.id | (slot.count & 0xff) << 16;

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}
