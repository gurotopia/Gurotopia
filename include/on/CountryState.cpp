#include "pch.hpp"

#include "CountryState.hpp"

void on::CountryState(ENetEvent& event) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    send_varlist(event.peer, {
        "OnCountryState",
        /* @todo add |showGuild when we add guild system; also add peer's real country */
        std::format("{}{}", pPeer->country, (pPeer->level[0] == 125) ? "|maxLevel" : "")
    }, pPeer->netid);
}