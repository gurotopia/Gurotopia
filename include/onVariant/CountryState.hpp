#pragma once

namespace on
{
    extern void CountryState(ENetEvent& event);                         // @note this player's flags, to everyone in their world
    extern void CountryStateOf(ENetPeer &to, const ::peer &who);        // @note someone's flags, to one player
}
