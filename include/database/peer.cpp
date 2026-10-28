#include "pch.hpp"
#include "items.hpp"
#include "world.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/CountryState.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/punch.hpp"
#include "tools/string.hpp"

#include "peer.hpp"

bool peer::exists(const std::string &growid)
{
    ::hStmt hStmt{ "SELECT 1 FROM peer WHERE growid = ? LIMIT 1" };

    MYSQL_BIND param = make_bind_in(growid); // WHERE
    hStmt.bind_param(&param);
    hStmt.execute();

    return (!mysql_stmt_store_result(hStmt.pStmt) && mysql_stmt_num_rows(hStmt.pStmt) > 0);
}

::blob slot::to_blob() const
{
    ::blob blob;
    blob.i16(this->id);
    blob.i16(this->count);

    return blob;
}

template<typename T>
void peer::mysql_insert(const std::string &column, const T &value)
{
    ::hStmt hStmt{ std::format("INSERT INTO peer ({}) VALUES (?)", column).c_str() };

    MYSQL_BIND param = make_bind_in(value); // VALUES
    hStmt.bind_param(&param);
    hStmt.execute();
}
template void peer::mysql_insert<signed>(const std::string&, const signed&);
template void peer::mysql_insert<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_insert<float>(const std::string&, const float&);
template void peer::mysql_insert<std::string>(const std::string&, const std::string&);

template<typename T>
void peer::mysql_update(const std::string &column, const T &value)
{
    ::hStmt hStmt{ std::format("UPDATE peer SET {} = ? WHERE growid = ?", column).c_str() };

    MYSQL_BIND params[2] = {
        make_bind_in(value),       // SET
        make_bind_in(this->growid) // WHERE
    };
    hStmt.bind_param(params);
    hStmt.execute();
}
template void peer::mysql_update<signed>(const std::string&, const signed&);
template void peer::mysql_update<unsigned>(const std::string&, const unsigned&);
template void peer::mysql_update<long long>(const std::string&, const long long&);
template void peer::mysql_update<float>(const std::string&, const float&);
template void peer::mysql_update<std::string>(const std::string&, const std::string&);

template<typename T>
T peer::mysql_select(const std::string &column, const std::string &arg, bool by_user_id)
{
    T value{};
    ::hStmt hStmt{ std::format("SELECT {}({}) FROM peer WHERE {} = ? LIMIT 1", arg, column, by_user_id ? "uid" : "growid").c_str() };

    MYSQL_BIND param = by_user_id ? make_bind_in(this->user_id) : make_bind_in(this->growid); // WHERE
    hStmt.bind_param(&param);

    u_long length = 0;
    MYSQL_BIND result = make_bind_out(value);
    result.length = &length;
    mysql_stmt_bind_result(hStmt.pStmt, &result);

    hStmt.execute();
    hStmt.fetch();
    if constexpr (std::is_same_v<T, std::string>)
        value.resize(length);

    return value;
}
/* since we will only select during mysql_select_all */ // @note add templates here if use select outside of this file.
template std::string peer::mysql_select<std::string>(const std::string&, const std::string&, bool); // @note tile_change.cpp (World Lock owner name)

void peer::mysql_select_all(bool by_user_id)
{
    this->user_id    = this->mysql_select<signed>("uid");
    this->growid     = this->mysql_select<std::string>("growid");
    this->password   = this->mysql_select<std::string>("password");
    this->created_at = this->mysql_select<std::time_t>("created_at", "UNIX_TIMESTAMP");
    this->gems       = this->mysql_select<long long>("gems");
    this->wl_storage = this->mysql_select<signed>("wl_storage");
    this->level[0]   = this->mysql_select<signed>("level");
    this->level[1]   = this->mysql_select<signed>("xp");
    auto inventory   = this->mysql_select<::blob>("inventory");

    if (inventory.size() >= sizeof(int)/*slot_size*/ + sizeof(short)/*size*/)
    {
        int pos{};
        inventory.read_i32(this->slot_size, pos);

        short size{};
        inventory.read_i16(size, pos);
        this->slots.resize(size);
        for (::slot &slot : this->slots)
        {
            if (!inventory.read_i16(slot.id, pos)) break;
            if (!inventory.read_i16(slot.count, pos)) break;
        }
    }
    else
    {
        this->slot_size = 16;
        this->slots.resize(3ull); // @note default starter items
        this->slots[0ull] = ::slot{18, 1};   // @note fist
        this->slots[1ull] = ::slot{32, 1};   // @note wrench
        this->slots[2ull] = ::slot{9640, 1}; // @note my first world lock
        this->mysql_update_all();
    }
}

::blob peer::serialize_inventory() const
{
    ::blob blob{};
    blob.i32(this->slot_size);
    blob.i16(this->slots.size());
    for (const ::slot &slot : this->slots)
    {
        blob.push_back(slot.to_blob());
    }
    return blob;
}

void peer::mysql_update_all()
{
    if (this->growid.empty()) return;

    ::hStmt hStmt{ "UPDATE peer SET inventory = ?, gems = ?, level = ?, xp = ?, wl_storage = ? WHERE growid = ?" };
    const ::blob inventory = this->serialize_inventory();
    const int level_value = this->level[0], xp_value = this->level[1];

    MYSQL_BIND params[6] = {
        make_bind_in(inventory),
        make_bind_in(this->gems),
        make_bind_in(level_value),
        make_bind_in(xp_value),
        make_bind_in(this->wl_storage),
        make_bind_in(this->growid)
    };
    hStmt.bind_param(params);
    hStmt.execute();
}

bool peer::load(const std::string &growid, const std::string password)
{
    this->growid = growid;
    if (!this->exists(growid))
    {
        return false;
    }
    this->mysql_select_all(false);
    if (password.empty() || this->password != password)
    {
        return false;
    }
    return true;
}

bool peer::load_by_uid(int uid)
{
    this->user_id = uid;
    this->mysql_select_all(true);
    return !this->growid.empty();
}

bool peer::create_account(const std::string &growid, const std::string &password)
{
    this->growid = growid;
    if (this->exists(growid))
    {
        return false;
    }
    this->mysql_insert("growid", growid);
    this->mysql_update("password", password);

    this->slots.resize(3ull); // @note default starter items
    this->slots[0ull] = ::slot{18, 1};   // @note fist
    this->slots[1ull] = ::slot{32, 1};   // @note wrench
    this->slots[2ull] = ::slot{9640, 1}; // @note my first world lock

    this->mysql_update_all();
    this->mysql_select_all(false);
    return true;
}

peer::~peer()
{
    this->mysql_update_all();
}

u_short peer::emplace(::slot slot) 
{
    if (auto it = std::ranges::find(this->slots, slot.id, &::slot::id); it != this->slots.end()) 
    {
        const u_short excess = std::max(0, (it->count + slot.count) - 200);
        it->count = std::min(it->count + slot.count, 200);
        if (it->count == 0)
        {
            const ::item &item = id_to_item(it->id);
            if (item.cloth_type != clothing::NONE) this->clothing[item.cloth_type] = 0;
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
        send_varlist(event.peer, { "OnPlayerLeveledUp", (unsigned)lvl });
        send_varlist(event.peer, { "OnParticleEffect", 46u, CL_Vec2f{this->pos.x + 16.0f, this->pos.y + 16.0f}, 0.0f, 0.0f });

        std::string message = std::format("{} is now level {}!", this->display_growid, lvl);
        send_varlist(event.peer, { "OnTalkBubble", this->netid, message, 0u });
        on::ConsoleMessage(event.peer, message);
    }
}

void peer::update_effects()
{
    this->punch_effect = 0;
    for (float cloth : this->clothing)
    {
        u_char punch_id = get_punch_id((u_int)cloth);
        if (punch_id != 0) // @note an actual change rather than no effect.
            this->punch_effect = punch_id;
    }
}

ENetHost *host;

void safe_disconnect_peers(int signal)
{
    peers("", PeerSearch::EVERYONE, [](auto *p) { enet_peer_disconnect(p, 0); });
    enet_host_flush(host);
}

gamePacket make_gamePacket(const enet_uint8 *data) 
{
    const int   *i32 = reinterpret_cast<const int*>(data);
    const u_int *u32 = reinterpret_cast<const u_int*>(data);
    const float *f32 = reinterpret_cast<const float*>(data);

    return gamePacket{
        .type  = i32[1],
        .netid = i32[2],
        .uid   = i32[3],
        .state = i32[4],
        .count = f32[5],
        .id    = i32[6],
        .pos   = ::pos{f32[7], f32[8]},
        .speed = ::pos{f32[9], f32[10]},
        .idk   = f32[11],
        .punch = ::pos{i32[12], i32[13]},
        .size  = u32[14]
    };
}

::blob compress_state(const gamePacket &gamePacket) 
{
    ::blob blob{};
    
    blob.i32(gamePacket.packet_create);
    blob.i32(gamePacket.type);
    blob.i32(gamePacket.netid);
    blob.i32(gamePacket.uid);
    blob.i32(gamePacket.state);
    blob.f32(gamePacket.count);
    blob.i32(gamePacket.id);
    blob.f32(gamePacket.pos.x);
    blob.f32(gamePacket.pos.y);
    blob.f32(gamePacket.speed.x);
    blob.f32(gamePacket.speed.y);
    blob.f32(gamePacket.idk);
    blob.i32(gamePacket.punch.x);
    blob.i32(gamePacket.punch.y);
    blob.i32(gamePacket.size);

    return blob;
}

void send_inventory_state(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::blob blob = compress_state(::gamePacket{
        .type = 0x09, // @note PACKET_SEND_INVENTORY_STATE
        .netid = pPeer->netid,
        .state = (int)gamePacketState::EXTENDED
    });
    blob.u8(0x01); // @note enable flag for big backpack

    blob.push_back(pPeer->serialize_inventory());

    ENetPacket *packet = enet_packet_create(blob.data().data(), blob.size(), ENET_PACKET_FLAG_RELIABLE);
	if (enet_peer_send(event.peer, 0, packet)) enet_packet_destroy(packet);
}
