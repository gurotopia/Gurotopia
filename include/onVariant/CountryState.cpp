#include "pch.hpp"
#include "commands/legendary.hpp"

#include "CountryState.hpp"

void on::CountryState(ENetEvent& event) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const std::string state = country_state_for(*pPeer); // @note country + maxLevel + doctor

    if (pPeer->netid == 0)
    {
        send_varlist(event.peer, { "OnCountryState", state }, pPeer->netid);
        return;
    }
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        send_varlist(&p, { "OnCountryState", state }, pPeer->netid);
    });
}

void on::CountryStateOf(ENetPeer &to, const ::peer &who)
{
    send_varlist(&to, { "OnCountryState", country_state_for(who) }, who.netid);
}
