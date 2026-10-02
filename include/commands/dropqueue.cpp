#include "pch.hpp"
#include "onVariant/SetBux.hpp"
#include <deque>
#include <chrono>
#include "database/world.hpp"
#include "dropqueue.hpp"

namespace
{
constexpr int  PER_BATCH = 25;                               // @note drops sent per batch
constexpr int  REMOVE_PER_BATCH = 15;                       // @note removals per batch (/cleardrops)
constexpr auto EVERY     = std::chrono::milliseconds(100);  // @note time between batches

struct queued
{
    std::string world;
    bool add;
    u_short id, count;
    ::pos pos;
    int uid;
    std::string by;
};
std::deque<queued> q{};

/* the player who asked for it if they're still in the world, else anyone in it */
ENetPeer *peer_for(const std::string &world, const std::string &by)
{
    ENetPeer *any = nullptr, *pref = nullptr;
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *t = static_cast<::peer*>(p.data);
        if (!any) any = &p;
        if (!by.empty() && t && t->growid == by) pref = &p;
    });
    return pref ? pref : any;
}
}

void queue_add_drop(const std::string &world, u_short id, u_short count, ::pos pos)
{
    q.push_back({ world, true, id, count, pos, 0, {} });
}

void queue_remove_drop(const std::string &world, int uid, const std::string &by_growid)
{
    q.push_back({ world, false, 0, 0, ::pos{}, uid, by_growid });
}

void dropqueue_cancel_adds(const std::string &world)
{
    std::erase_if(q, [&](const queued &d) { return d.add && d.world == world; });
}

/* called every loop of the server: sends the next batch when it's time */
void dropqueue_pump()
{
    static auto next = std::chrono::steady_clock::now();
    if (q.empty()) return;
    const auto now = std::chrono::steady_clock::now();
    if (now < next) return;
    next = now + EVERY;

    ENetPeer *resync = nullptr;
    for (int adds = 0, removes = 0; !q.empty() && adds < PER_BATCH && removes < REMOVE_PER_BATCH; )
    {
        const queued d = q.front();
        q.pop_front();
        d.add ? ++adds : ++removes;

        auto w = std::ranges::find(worlds, d.world, &::world::name);
        if (w == worlds.end()) continue; // @note world closed meanwhile
        ENetPeer *p = peer_for(d.world, d.by);

        if (d.add)
        {
            if (p)
            {
                ENetEvent ev{};
                ev.peer = p;
                add_object(ev, ::slot(static_cast<short>(d.id), static_cast<short>(d.count)), d.pos, *w);
            }
            else w->objects.emplace_back(::object(d.id, d.count, d.pos, ++w->last_object_uid));
        }
        else
        {
            auto o = std::ranges::find(w->objects, static_cast<u_int>(d.uid), &::object::uid);
            if (o == w->objects.end()) continue; // @note someone already picked it up
            w->objects.erase(o);
            if (p)
            {
                ENetEvent ev{};
                ev.peer = p;
                item_change_object(ev, ::gamePacket{ .netid = static_cast<::peer*>(p->data)->netid, .uid = (int)0xffffffff, .id = d.uid });
                resync = p;
            }
        }
    }
    if (resync) // @note the removal looks like a pickup to that client, so give it the real backpack and gems back
    {
        ENetEvent ev{};
        ev.peer = resync;
        send_inventory_state(ev);
        on::SetBux(ev);
    }
}
