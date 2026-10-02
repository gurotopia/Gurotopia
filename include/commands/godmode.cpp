#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/items.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "godmode.hpp"

extern std::vector<u_int> harmful_type_pos; // @note filled by decode_items()
extern u_int im_file_offset;

static constexpr u_int HASH_NORMAL = 2966867045u; // @note same as the hardcoded value in tankIDName.cpp

static std::vector<u_char> im_data_god{};
static u_int hash_god = 0;

/* items.dat checksum */
static u_int proton_hash(const u_char *data, std::size_t len)
{
    u_int acc = 0x55555555u;
    for (std::size_t i = 0; i < len; ++i) acc = (acc >> 27) + (acc << 5) + data[i];
    return acc;
}

static u_int proton_hash_signed(const u_char *data, std::size_t len)
{
    u_int acc = 0x55555555u;
    for (std::size_t i = 0; i < len; ++i) acc = (acc >> 27) + (acc << 5) + static_cast<u_int>(static_cast<int>(static_cast<signed char>(data[i])));
    return acc;
}

/* a copy of items.dat where spikes, lava and other painful blocks are plain blocks */
static void build()
{
    if (!im_data_god.empty()) return;
    im_data_god = im_data;
    for (u_int p : harmful_type_pos)
        if (p < im_data_god.size()) im_data_god[p] = type::FOREGROUND;

    const u_int computed_normal = proton_hash(im_data.data() + im_file_offset, im_data.size() - im_file_offset);
    const u_int computed_signed = proton_hash_signed(im_data.data() + im_file_offset, im_data.size() - im_file_offset);
    (void)0; // @note debug line removed
    hash_god = (computed_signed == HASH_NORMAL)
        ? proton_hash_signed(im_data_god.data() + im_file_offset, im_data_god.size() - im_file_offset)
        : proton_hash(im_data_god.data() + im_file_offset, im_data_god.size() - im_file_offset);
    (void)0; // @note debug line removed
}

const std::vector<u_char> &items_for(ENetEvent &event)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    if (!p || !p->god_mode) return im_data;
    build();
    return im_data_god;
}

u_int items_hash_for(const ::peer &p)
{
    if (!p.god_mode) return HASH_NORMAL;
    build();
    return hash_god;
}

/* called by /god after it toggles god_mode */
void god_items_changed(ENetEvent &event)
{
    ::peer *p = static_cast<::peer*>(event.peer->data);
    p->mysql_update("god", static_cast<signed>(p->god_mode ? 1 : 0));

    if (p->netid != 0) { p->god_rejoin = p->recent_worlds.back(); p->god_rejoin_pos = p->pos; }
    const std::vector<u_char> &data = items_for(event);
    enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
    on::ConsoleMessage(event.peer, p->god_mode
        ? "`2Spikes and lava can't hurt you now.`` `oIf they still do, re-enter the world or log in again.``"
        : "`oSpikes and lava are dangerous again. Re-enter the world or log in again if they're not.``");
}


/* runs once a second: puts players back in their world after /god reloaded items.dat */
void god_rejoin_tick(std::time_t now)
{
    peers("", peer_condition::PEER_ALL, [now](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->god_rejoin_at == 0 || now < t->god_rejoin_at) return;

        const std::string world = t->god_rejoin;
        const ::pos at = t->god_rejoin_pos;
        t->god_rejoin.clear();
        t->god_rejoin_at = 0;
        if (world.empty()) return;

        if (t->netid != 0) return; // @note the game already went back into a world by itself
        ENetEvent ev{};
        ev.peer = &p;
        action::join_request(ev, "", world);
        t->pos = at;
        send_varlist(&p, { "OnSetPos", CL_Vec2f{ at.x, at.y } }, t->netid);
    });
}