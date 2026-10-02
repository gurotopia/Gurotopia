#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "database/world.hpp"
#include "nuke_confirm.hpp"

void nuke_confirm(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER) return;
    if (pPeer->netid == 0) return;

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    const std::string world_name = world->name;

    blast::thermonuclear(*world);
    world->signs.clear();
    world->trees.clear();
    world->nuked = true;

    // gather everyone here before we start moving people around
    std::vector<ENetPeer*> staying;
    std::vector<ENetPeer*> leaving;

    peers(world_name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *pOthers = static_cast<::peer*>(p.data);
        if (!pOthers) return;

        on::ConsoleMessage(&p, std::format("`5[```4{}`` `5has been nuked from existence!```5]``", world_name));

        if (pOthers->user_id == pPeer->user_id) return;          // handled last
        if (pOthers->role >= MODERATOR) staying.push_back(&p);
        else                            leaving.push_back(&p);
    });

    // non-staff are sent to the world menu
    for (ENetPeer *p : leaving)
    {
        ENetEvent ev{};
        ev.peer = p;
        action::quit_to_exit(ev, "", false);
    }

    // staff reload the world so they see the cleared map
    for (ENetPeer *p : staying)
    {
        ENetEvent ev{};
        ev.peer = p;
        action::quit_to_exit(ev, "", true);
        action::join_request(ev, "", world_name);
    }

    action::quit_to_exit(event, "", true);
    action::join_request(event, "", world_name);
}
