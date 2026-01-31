#pragma once
#ifndef ONSPAWN_HPP
#define ONSPAWN_HPP

    namespace on
    {
        extern void Spawn(ENetPeer &peer, signed netID, signed userID, ::pos posXY, std::string name, std::string country, bool mstate, bool smstate, bool local) ;
    }

#endif