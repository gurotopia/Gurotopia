#pragma once
#include "onVariant/ConsoleMessage.hpp"

/* a talk bubble above the player's own head (only they see it). outside a world it goes to chat instead. */
inline void tell(ENetPeer *peer, const std::string &text)
{
    ::peer *p = static_cast<::peer*>(peer->data);
    if (p && p->netid != 0) send_varlist(peer, { "OnTalkBubble", p->netid, text, 0u });
    else on::ConsoleMessage(peer, text);
}

/* while a consumable is used on another player (set in consumables.cpp), that player's head says the text */
inline ::peer *g_consume_target = nullptr;
inline const ::peer &speaker(const ::peer &user) { return (g_consume_target && &user != g_consume_target) ? *g_consume_target : user; }