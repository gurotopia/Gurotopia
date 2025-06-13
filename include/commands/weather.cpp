#include "pch.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "weather.hpp"

int get_weather_id(unsigned item_id)
{
    switch (item_id)
    {
        case 830: return 1; // @note beach (blast)
        case 934: return 2; // @note night
        case 946: return 3; // @note arid

        case 984: return 5; // @note rainy city
        case 1060: return 6; // @note harvest moon (blast)
        case 1136: return 7; // @note mars (blast)
        case 1210: return 8; // @note spooky

        case 1490: return 10; // @note nothingness
        case 1364: return 11; // @note snowy

        case 1532: return 14; // @note undersea (blast)
        case 1750: return 15; // @note warp speed

        case 2046: return 17; // @note comet
        case 2284: return 18; // @note party
        case 2744: return 19; // @note pineapples
        case 3252: return 20; // @note snowy night
        case 3446: return 21; // @note spring
        case 3534: return 22; // @note howling sky

        case 3694: return 23; // @note heatwave (24-28)
        
        case 3832: return 29; // @note stuff
        case 4242: return 30; // @note pagoda
        case 4486: return 31; // @note apocalypse
        case 4774: return 32; // @note jungle (blast)
        case 4892: return 33; // @note balloon warz
        case 5000: return 34; // @note background
        case 5112: return 35; // @note autumn
        case 5654: return 36; // @note valentine's
        case 5716: return 37; // @note St. paddy's

        case 6854: return 42; // @note digital rain
        case 7380: return 43; // @note monochrome (blast)
        case 7644: return 44; // @note frozen cliffs
        case 8556: return 45; // @note surg world (blast)
        case 8738: return 46; // @note bountiful (blast)
        case 8836: return 47; // @note stargazing
        case 8896: return 48; // @note meteor shower

        case 10286: return 51; // @note celebrity hills
        case 11880: return 59; // @note plaza
        case 12054: return 60; // @note nebula
        case 12056: return 61; // @note protostar landing
        case 12408: return 62; // @note dark mountains

        case 12844: return 64; // @note Mt. growmore
        case 13004: return 65; // @note crack in reality
        case 13070: return 66; // @note 年兽 (nián) mountains

        case 13640: return 69; // @note realm of spirits
        case 13690: return 70; // @note black hole
        case 14032: return 71; // @note raining gems
        case 14094: return 72; // @note holiday heaven

        case 14598: return 76; // @note atlantis
        case 14802: return 77; // @note pinuski's petal purrfect haven
        case 14896: return 78; // @note candyland blast
        case 15150: return 79; // @note dragon's keep
        case 15240: return 80; // @note emerald city
    }
    return 0; // @note sunny
}

void weather(ENetEvent& event, const std::string_view text)
{
    std::string id{ text.substr(sizeof("weather ")-1) };
    gt_packet(*event.peer, false, 0, {
        "OnSetCurrentWeather",
        stoi(id)
    });
}
