#include "pch.hpp"
#include "tools/bubble.hpp"
#include "trade.hpp"
#include "friendsys.hpp"
#include "pull.hpp"
#include "staff.hpp"
#include "legendary.hpp"
#include "playeractions.hpp"

bool wrench_action(ENetEvent &event, const ::hPipe &hPipe)
{
    const std::string btn = hPipe["buttonClicked"];
    if (staff_popup(event, hPipe)) return true;
    if (btn == "title_edit") { titles_open(event); return true; } // @note wrench yourself > Title
    if (btn == "title_toggle") { title_toggle(event); return true; } // @note quick on/off button
    if (btn != "trade" && btn != "friend_add" && btn != "pull_player") return false;

    ::peer *me = static_cast<::peer*>(event.peer->data);
    const int netid = std::atoi(hPipe["netID"].c_str());
    ENetPeer *target = nullptr;
    if (me->netid != 0)
        peers(me->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { const ::peer *t = static_cast<::peer*>(p.data); if (t && t->netid == netid) target = &p; });
    if (!target) { tell(event.peer, "`4That player isn't here anymore.``"); return true; }

    if (btn == "trade") trade_request(event, *target);
    else if (btn == "friend_add") friend_request(event, *target);
    else pull_player(event, *target);
    return true;
}
