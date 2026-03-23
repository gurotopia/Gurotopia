#include "pch.hpp"

#include "CountryState.hpp"

void on::CountryState(ENetEvent& event) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, true, 0, {
        "OnCountryState",
        /* @todo add |showGuild when we add guild system; also add peer's real country */
        std::format("{}{}", pPeer->country, (pPeer->level[0] == 125) ? "|maxLevel" : "").c_str()
    });
}