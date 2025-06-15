#include "pch.hpp"
#include "network/packet.hpp"
#include "join_request.hpp"

#include "tools/string_view.hpp"

void logging_in(ENetEvent event, const std::string& header)
{
    auto &peer = _peer[event.peer];
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes[2zu] == "ltoken")
    {
        std::string decoded = base64Decode(pipes[3zu]);
        if (std::size_t pos = decoded.find("growId="); pos != std::string::npos) 
        {
            pos += sizeof("growId=")-1zu;
            const std::size_t ampersand = decoded.find('&', pos);
            peer->ltoken[0] = strdup(decoded.substr(pos, ampersand - pos).c_str());
        }

        if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
        {
            pos += sizeof("password=")-1zu;
            peer->ltoken[1] = strdup(decoded.substr(pos).c_str());
        }
    }
    peer->read(peer->ltoken[0]);
    gt_packet(*event.peer, false, 0, {
        "OnSuperMainStartAcceptLogonHrdxs47254722215a",
        799452297u, // @note items.dat
        "ubistatic-a.akamaihd.net",
        "0098/41415312412/cache/",
        "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro com.proziro.zacro com.slash.gamebuster",
        "proto=216|choosemusic=audio/mp3/about_theme.mp3|active_holiday=0|wing_week_day=0|ubi_week_day=0|server_tick=55563760|clash_active=1|drop_lavacheck_faster=1|isPayingUser=1|usingStoreNavigation=1|enableInventoryTab=1|bigBackpack=1",
        0u // @note player_tribute.dat (not using)
    });
}