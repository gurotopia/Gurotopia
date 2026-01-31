#include "pch.hpp"
#include "tools/string.hpp"

#include "Spawn.hpp"

void on::Spawn(ENetPeer &peer, signed netID, signed userID, ::pos posXY, std::string name, std::string country, bool mstate, bool smstate, bool local) 
{
    packet::create(peer, false, -1/* ff ff ff ff */, {
        "OnSpawn",
        std::format(
            "spawn|avatar\nnetID|{}\nuserID|{}\ncolrect|0|0|20|30\nposXY|{}|{}\nname|{}``\ncountry|{}\ninvis|0\nmstate|{}\nsmstate|{}\nonlineID|\n{}",
            netID, userID, posXY.x, posXY.y, name, country, to_char(mstate), to_char(smstate), (local) ? "type|local\n" : "").c_str()
    });
}