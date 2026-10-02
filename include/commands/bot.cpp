#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/Spawn.hpp"
#include "database/world.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/items.hpp"
#include "bot.hpp"

std::vector<::bot> bots{};

/* negative user ids so they never clash with real accounts */
static int next_bot_uid = -1;

/* send every bot in this world to the peer that just walked in */
void spawn_bots_for(ENetEvent& event, const std::string &world_name)
{
    for (::bot &b : bots)
    {
        if (b.world != world_name) continue;
        on::Spawn(*event.peer, b.netid, b.user_id, b.pos, b.name, b.country, 0, false, false);
        on::SetClothing(*event.peer, b.netid, b.skin, b.state, &b.clothing);
        send_varlist(event.peer, { "OnSetPos", CL_Vec2f{ b.pos.x, b.pos.y } }, b.netid);
    }
}

/* /bot                 -> lists bots in this world
 * /bot add {name}      -> creates one at your position
 * /bot remove {name}   -> removes it
 * /bot clear           -> removes every bot in this world
 * /bot say {name} {msg}-> makes one talk
 */
void bot_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    std::string sub{}, rest{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        sub = arg.substr(0, sp);
        rest = arg.substr(sp + 1);
    }
    else sub = arg;

    for (char &c : sub) c = std::tolower(static_cast<unsigned char>(c));

    /* no argument: list what's here */
    if (sub.empty())
    {
        std::string names{};
        std::size_t count = 0;
        for (const ::bot &b : bots)
        {
            if (b.world != world->name) continue;
            if (!names.empty()) names += "`o, ";
            names += std::format("`w{}``", b.name);
            ++count;
        }

        if (count == 0)
            on::ConsoleMessage(event.peer, "`oNo bots here. `$/bot add {name}`o to make one.");
        else
            on::ConsoleMessage(event.peer, std::format("`o{} bot{} here: {}", count, count == 1 ? "" : "s", names));
        return;
    }

    if (sub == "add")
    {
        if (rest.empty() || rest.length() > 18)
        {
            on::ConsoleMessage(event.peer, "`4Usage: /bot add {name}  (max 18 characters)``");
            return;
        }

        for (const ::bot &b : bots)
            if (b.world == world->name && b.name == rest)
            {
                on::ConsoleMessage(event.peer, "`4A bot with that name is already here.``");
                return;
            }

        ::bot b{};
        b.netid = ++world->netid_counter;
        b.user_id = next_bot_uid--;
        b.name = std::format("`w{}``", rest);
        b.world = world->name;
        b.pos = pPeer->pos;

        bots.emplace_back(b);

        // show it to everyone standing here
        peers(world->name, PEER_SAME_WORLD, [&b](ENetPeer &p)
        {
            on::Spawn(p, b.netid, b.user_id, b.pos, b.name, b.country, 0, false, false);
            on::SetClothing(p, b.netid, b.skin, b.state, &b.clothing);
            send_varlist(&p, { "OnSetPos", CL_Vec2f{ b.pos.x, b.pos.y } }, b.netid);
        });

        on::ConsoleMessage(event.peer, std::format("`2Spawned bot `w{}``.``", rest));
        return;
    }

    if (sub == "remove")
    {
        if (rest.empty())
        {
            on::ConsoleMessage(event.peer, "`4Usage: /bot remove {name}``");
            return;
        }

        for (auto it = bots.begin(); it != bots.end(); ++it)
        {
            if (it->world != world->name) continue;
            if (it->name.find(rest) == std::string::npos) continue;

            const std::string netid = std::format("netID|{}\n", it->netid);
            const std::string pId = std::format("pId|{}\n", it->user_id);

            peers(world->name, PEER_SAME_WORLD, [netid, pId](ENetPeer &p)
            {
                send_varlist(&p, { "OnRemove", netid, pId });
            });

            bots.erase(it);
            on::ConsoleMessage(event.peer, std::format("`2Removed bot `w{}``.``", rest));
            return;
        }

        on::ConsoleMessage(event.peer, std::format("`4No bot named `w{}`` here.``", rest));
        return;
    }

    if (sub == "clear")
    {
        std::size_t removed = 0;
        for (auto it = bots.begin(); it != bots.end(); )
        {
            if (it->world != world->name) { ++it; continue; }

            const std::string netid = std::format("netID|{}\n", it->netid);
            const std::string pId = std::format("pId|{}\n", it->user_id);

            peers(world->name, PEER_SAME_WORLD, [netid, pId](ENetPeer &p)
            {
                send_varlist(&p, { "OnRemove", netid, pId });
            });

            it = bots.erase(it);
            ++removed;
        }

        on::ConsoleMessage(event.peer, std::format("`2Removed `w{}`` bot{}.``", removed, removed == 1 ? "" : "s"));
        return;
    }

    if (sub == "say")
    {
        std::string bot_name{}, message{};
        if (const std::size_t sp = rest.find(' '); sp != std::string::npos)
        {
            bot_name = rest.substr(0, sp);
            message = rest.substr(sp + 1);
        }

        if (bot_name.empty() || message.empty())
        {
            on::ConsoleMessage(event.peer, "`4Usage: /bot say {name} {message}``");
            return;
        }

        for (const ::bot &b : bots)
        {
            if (b.world != world->name) continue;
            if (b.name.find(bot_name) == std::string::npos) continue;

            const std::string bubble = std::format("CP:0_PL:0_OID:_player_chat=`${}``", message);
            const std::string line = std::format("CP:0_PL:0_OID:_CT:[W]_ `6<{}>`` `${}``", b.name, message);

            peers(world->name, PEER_SAME_WORLD, [&b, bubble, line](ENetPeer &p)
            {
                send_varlist(&p, { "OnTalkBubble", b.netid, bubble });
                on::ConsoleMessage(&p, line);
            });
            return;
        }

        on::ConsoleMessage(event.peer, std::format("`4No bot named `w{}`` here.``", bot_name));
        return;
    }

    /* visual toggles and clothing */
    if (sub == "ghost" || sub == "invis" || sub == "wear")
    {
        std::string bot_name{}, extra{};
        if (const std::size_t sp = rest.find(' '); sp != std::string::npos)
        {
            bot_name = rest.substr(0, sp);
            extra = rest.substr(sp + 1);
        }
        else bot_name = rest;

        if (bot_name.empty())
        {
            on::ConsoleMessage(event.peer, std::format("`4Usage: /bot {} {{name}}{}``", sub, sub == "wear" ? " {itemID}" : ""));
            return;
        }

        for (::bot &b : bots)
        {
            if (b.world != world->name) continue;
            if (b.name.find(bot_name) == std::string::npos) continue;

            if (sub == "ghost")
            {
                b.state ^= S_GHOST;
                on::ConsoleMessage(event.peer, std::format("`2`w{}`` ghost {}.``", bot_name, (b.state & S_GHOST) ? "on" : "off"));
            }
            else if (sub == "invis")
            {
                b.state ^= S_INVISIBLE;
                on::ConsoleMessage(event.peer, std::format("`2`w{}`` invisible {}.``", bot_name, (b.state & S_INVISIBLE) ? "on" : "off"));
            }
            else
            {
                std::string id_str = extra;
                std::erase_if(id_str, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
                if (id_str.empty())
                {
                    on::ConsoleMessage(event.peer, "`4Usage: /bot wear {name} {itemID}``");
                    return;
                }

                signed id{};
                try { id = std::stoi(id_str); }
                catch (...) { on::ConsoleMessage(event.peer, "`4Not a valid item id.``"); return; }

                const ::item &it = id_to_item(static_cast<short>(id));
                if (it.cloth_type == clothing::NONE)
                {
                    on::ConsoleMessage(event.peer, "`4That item can't be worn.``");
                    return;
                }

                b.clothing[it.cloth_type] = static_cast<float>(id);
                on::ConsoleMessage(event.peer, std::format("`2`w{}`` is now wearing item `w{}``.``", bot_name, id));
            }

            // push the change to everyone in the world
            peers(world->name, PEER_SAME_WORLD, [&b](ENetPeer &p)
            {
                on::SetClothing(p, b.netid, b.skin, b.state, &b.clothing);
            });
            return;
        }

        on::ConsoleMessage(event.peer, std::format("`4No bot named `w{}`` here.``", bot_name));
        return;
    }

    on::ConsoleMessage(event.peer, "`4Usage: /bot [add|remove|clear|say|ghost|invis|wear]``");
}
