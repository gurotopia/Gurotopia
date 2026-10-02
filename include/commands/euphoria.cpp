#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "weather.hpp"
#include "euphoria.hpp"

namespace
{
struct party_world
{
    int points = 0;
    bool started = false, euphoria = false;
    std::unordered_map<std::string, int> by{};       // @note points per player
    std::vector<std::string> weather_sent{};          // @note players who already got the party weather
};
std::unordered_map<std::string, party_world> party{};

/* points per item used */
int points_for(short id)
{
    switch (id)
    {
        case 4378: case 4370: return 8;   // @note Party Cake, Party Popper
        case 2306: return 10;             // @note Party-In-A-Box
        case 9264: return 50;             // @note Anniversary Skyrocket
        case 4366: return 150;            // @note Party Screamer
        case 2288: return 4;              // @note Party Confetti (100 = 400)
    }
    return 0;
}

int party_weather()
{
    static int w = -1;
    if (w == -1)
    {
        w = 0;
        for (const ::item &it : items)
            if (it.raw_name == "Weather Machine - Party") { w = get_weather_id(it.id); break; }
    }
    return w;
}

void banner(const std::string &world, const std::string &text)
{
    peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        send_varlist(&p, { "OnAddNotification", "interface/large/special_event.rttex", text, "audio/cumbia_horns.wav", 0u });
        on::ConsoleMessage(&p, text);
    });
}
}

void euphoria_used(ENetEvent &event, ::world &w, short id, int used)
{
    const int pts = points_for(id) * used;
    if (pts <= 0) return;
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    party_world &pw = party[w.name];
    pw.points += pts;
    pw.by[pPeer->growid] += pts;
    on::ConsoleMessage(event.peer, std::format("`2+{} party points``  (world: `w{}``/5500, you: `w{}``)", pts, pw.points, pw.by[pPeer->growid]));

    if (!pw.started && pw.points >= 1500)
    {
        pw.started = true;
        banner(w.name, "`2Party Started!:`` `oThis world is now a party world!``");
    }
    if (!pw.euphoria && pw.points >= 5500)
    {
        pw.euphoria = true;
        banner(w.name, "`2EUPHORIA!:`` `oEveryone who added `w500`` party points gets a `wGolden Party-In-A-Box``!``");
        peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &p)
        {
            ::peer *t = static_cast<::peer*>(p.data);
            if (!t || pw.by[t->growid] < 500) return;
            ENetEvent ev{};
            ev.peer = &p;
            modify_item_inventory(ev, ::slot(7672, 1));
            tell(&p, "`2You got a `wGolden Party-In-A-Box``!``");
        });
    }
}

/* every second: party weather for newcomers, reset when the world is empty */
void euphoria_tick(std::time_t)
{
    for (auto it = party.begin(); it != party.end(); )
    {
        std::vector<ENetPeer*> in{};
        peers(it->first, PEER_SAME_WORLD, [&](ENetPeer &p) { in.push_back(&p); });
        if (in.empty()) { it = party.erase(it); continue; }

        party_world &pw = it->second;
        std::erase_if(pw.weather_sent, [&](const std::string &g)
        {
            return std::ranges::none_of(in, [&](ENetPeer *p) { const ::peer *t = static_cast<::peer*>(p->data); return t && t->growid == g; });
        });
        if (pw.started)
            for (ENetPeer *p : in)
            {
                const ::peer *t = static_cast<::peer*>(p->data);
                if (!t || std::ranges::find(pw.weather_sent, t->growid) != pw.weather_sent.end()) continue;
                pw.weather_sent.push_back(t->growid);
                send_varlist(p, { "OnSetCurrentWeather", party_weather() });
            }
        ++it;
    }
}
