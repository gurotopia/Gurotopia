#include "pch.hpp"
#include "onVariant/EmoticonDataChanged.hpp"
#include "onVariant/Spawn.hpp"
#include "onVariant/BillboardChange.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/CountryState.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/jammers.hpp"
#include "commands/weather.hpp"
#include "tools/time.hpp"

#include "commands/bot.hpp"
#include "action/quit_to_exit.hpp"
#include "commands/curse.hpp"
#include "commands/lockaccess.hpp"
#include "join_request.hpp"
#include "commands/legendary.hpp"

static std::unordered_map<int, ::pos> arrivals{}; // @note where an account should appear instead of the door (summon, pull)
void arrive_at(int user_id, ::pos at) { arrivals[user_id] = at; }

void action::join_request(ENetEvent& event, const std::string& header, const std::string_view world_name = "") 
{
    try 
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        ::hPipe hPipe{ header };

        std::string name =  hPipe["name"];
        if (name.empty() && !world_name.empty()) name = world_name;

        if (name.length() > 24) throw std::runtime_error(""); // @note impossible unless using a proxy since client caps at 24
        if (!alnum(name)) throw std::runtime_error("Sorry, spaces and special characters are not allowed in world or door names.  Try again.");

        for (char &c : name) c = std::toupper(c); // @note start -> START

        if (pPeer->curse_until > std::time(nullptr) && name != "HELL")
        {
            on::ConsoleMessage(event.peer, std::format("`4You are cursed!`` You can't leave `wHELL`` for another `w{}``.", curse_time_left(*pPeer)));
            name = "HELL";
        }
        if (name == "HELL" && pPeer->curse_until <= std::time(nullptr) && pPeer->role < MODERATOR)
            throw std::runtime_error("`4Only the cursed may enter HELL.``");

        if (pPeer->netid != 0) action::quit_to_exit(event, "", true); // @note still in a world: leave it properly first
        
        auto it = std::ranges::find(worlds, name, &::world::name);
        if (it == worlds.end()) 
            it = worlds.emplace(worlds.end(), name);
            
        ::world &world = *it;

        if (world.nuked && pPeer->role < MODERATOR && world.name != "HELL")
            throw std::runtime_error("`4That world has been nuked and is closed.``");
        {
            ::blob blob = compress_state(::gamePacket{ .type = 0x04, /*PACKET_SEND_MAP_DATA*/ .state = state::S_EXTENDED });
            blob.push_back(world.serialize());

            ENetPacket *packet = enet_packet_create(blob.data().data(), blob.size(), ENET_PACKET_FLAG_RELIABLE);
            if (enet_peer_send(event.peer, 0, packet)) enet_packet_destroy(packet);
        } // @note delete blob
        {
            std::string *this_world = std::ranges::find(pPeer->recent_worlds, world.name);
            std::string *end = pPeer->recent_worlds.end();
            std::string *first = this_world != end ? this_world : pPeer->recent_worlds.begin();

            std::rotate(first, first + 1, end);
            pPeer->recent_worlds.back() = world.name;
        } // @note delete name, first
        on::EmoticonDataChanged(event);

        pPeer->display_growid = display_name_for(*pPeer, world); // @note /nick included // @note owner green, access light green, staff pink/gold

        pPeer->rest_pos = world.spawn;
        if (auto at = arrivals.find(pPeer->user_id); at != arrivals.end()) { pPeer->rest_pos = at->second; arrivals.erase(at); }
        pPeer->pos = pPeer->rest_pos;
        leave_flush(world.name, pPeer->user_id); // @note if this account left this world a moment ago, finish that first

        if (world.visitors == 0) world.netid_counter = 0; // @note nobody here, safe to start fresh
        pPeer->netid = ++world.netid_counter;
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer, &world](ENetPeer& peer/*send to everyone in world*/) 
        {
            ::peer *pOthers = static_cast<::peer*>(peer.data); // @note everyone in world's peer.data
            
            const bool other_hidden = (pOthers->state & S_INVISIBLE) == S_INVISIBLE, me_hidden = (pPeer->state & S_INVISIBLE) == S_INVISIBLE; // @note /invis players stay hidden
            if (pOthers->user_id != pPeer->user_id) // @note hidden players still see everyone; nobody sees them
            {
                if (!other_hidden) on::Spawn(*event.peer, pOthers->netid, pOthers->user_id, pOthers->pos, pOthers->display_growid, country_state_for(*pOthers), pOthers->role, pOthers->role >= DEVELOPER, false);
                if (me_hidden) return; // @note /invis: nobody gets your spawn, name or entered message
                on::Spawn(peer, pPeer->netid, pPeer->user_id, pPeer->rest_pos, pPeer->display_growid, country_state_for(*pPeer), pPeer->role, pPeer->role >= DEVELOPER, false);
                // clothing is sent after all spawns are processed
                on::SetClothing(peer, *pPeer);
                send_varlist(&peer, { "OnNameChanged", pPeer->display_growid }, pPeer->netid); // @note redraw your title look for them
                on::CountryStateOf(peer, *pPeer); // @note title flags for the others
                if (!g_silent_move) on::ConsoleMessage(&peer, std::format("`5<{} entered, `w{}`` others here>``", pPeer->display_growid, world.visitors));
            }
            

            if (pOthers->user_id != pPeer->user_id && !other_hidden && !me_hidden && !g_silent_move) // @note the reason this is here is cause we need the peer's OnSpawn to happen before OnTalkBubble
            {
                send_varlist(&peer, {
                    "OnTalkBubble",
                    pPeer->netid,
                    std::format("`5<{} entered, `w{}`` others here>``", pPeer->display_growid, world.visitors),
                    1u
                });
            }
        });
        // second pass: now that every spawn is out, send each player's look to the newcomer
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer](ENetPeer& peer)
        {
            ::peer *pOthers = static_cast<::peer*>(peer.data);
            if (pOthers->user_id == pPeer->user_id || (pOthers->state & S_INVISIBLE) == S_INVISIBLE) return;
            on::SetClothing(*event.peer, *pOthers);
            send_varlist(event.peer, { "OnNameChanged", pOthers->display_growid }, pOthers->netid); // @note redraw their title look
            on::CountryStateOf(*event.peer, *pOthers); // @note their Dr./max level flags
            send_varlist(event.peer, { "OnSetPos", CL_Vec2f{ pOthers->pos.x, pOthers->pos.y } }, pOthers->netid);
        });
        spawn_bots_for(event, world.name);
        on::Spawn(*event.peer, pPeer->netid, pPeer->user_id, pPeer->rest_pos, pPeer->display_growid, country_state_for(*pPeer), pPeer->role, pPeer->role >= DEVELOPER, true);
        on::CountryState(event); // @note your own title flags (the Legendary look needs this)

        if (pPeer->billboard.id != 0) on::BillboardChange(event); // @note don't waste memory if billboard is empty.

        send_varlist(event.peer, {
            "OnSetPos", 
            CL_Vec2f{pPeer->rest_pos.x, pPeer->rest_pos.y}
        }, pPeer->netid);

        send_varlist(event.peer, { "OnNameChanged", pPeer->display_growid }, pPeer->netid); // @note redraw your own title look
        on::CountryState(event);
        if (!g_silent_move) on::ConsoleMessage(event.peer, 
            std::format(
                "World `w{}`` entered.  There are `w{}`` other people here, `w{}`` online.", 
                world.name, world.visitors, peers().size()
            )
        );
                if (world.owner != 0)
        {
            std::string owner_name{};
            signed owner_role = 0;

            // prefer the online peer list - no database round trip at all
            peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
            {
                ::peer *o = static_cast<::peer*>(p.data);
                if (o && o->user_id == world.owner)
                {
                    owner_name = o->growid;
                    owner_role = o->role;
                }
            });

            if (owner_name.empty()) owner_name = std::format("user #{}", world.owner);

            const std::string owner_display =
                (owner_role >= 2) ? std::format("`6@{}``", owner_name) :
                (owner_role == 1) ? std::format("`5@{}``", owner_name) :
                                    std::format("`w{}``", owner_name);

            on::ConsoleMessage(event.peer, (std::format("`5[`w{}`` is owned by {}`5]``", world.name, owner_display) + jam_suffix(world.name)));
        }

        ++world.visitors;
        on::SetClothing(*event.peer);
        on::CountryState(event);
        send_varlist(event.peer, { "OnSetCurrentWeather", world.base_weather });
    }
    catch (const std::exception& exc)
    {
        if (event.peer->data) std::printf("[join] %s could not enter a world: %s\n", static_cast<::peer*>(event.peer->data)->growid.c_str(), exc.what()[0] ? exc.what() : "(no message)");
        send_varlist(event.peer, { "OnFailedToEnterWorld" });
        if (const std::string_view msg{ exc.what() }; !msg.empty()) on::ConsoleMessage(event.peer, std::string{ msg });
        return;
    }
}
