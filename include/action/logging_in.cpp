#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "join_request.hpp"

#include "tools/string_view.hpp"

void logging_in(ENetEvent event, const std::string& header)
{
    std::call_once(_peer[event.peer]->logging_in, [&]() 
    {
        std::vector<std::string> pipes = readch(header, '|');
        if (pipes[2] == "ltoken")
        {
            std::string decoded = base64Decode(pipes[3]);
            if (std::size_t pos = decoded.find("growId="); pos != std::string::npos) 
            {
                pos += sizeof("growId=")-1ull;
                const std::size_t ampersand = decoded.find('&', pos);
                _peer[event.peer]->ltoken[0] = strdup(decoded.substr(pos, ampersand - pos).c_str());
            }

            if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
            {
                pos += sizeof("password=")-1ull;
                _peer[event.peer]->ltoken[1] = strdup(decoded.substr(pos).c_str());
            }
        }
        _peer[event.peer]->read(_peer[event.peer]->ltoken[0]);
        gt_packet(*event.peer, false, 0, {
            "OnSuperMainStartAcceptLogonHrdxs47254722215a",
            0u,
            "ubistatic-a.akamaihd.net",
            "0098/49846198461/cache/",
            "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro com.proziro.zacro com.slash.gamebuster",
            "proto=216|choosemusic=audio/mp3/about_theme.mp3|active_holiday=0|wing_week_day=0|ubi_week_day=0|server_tick=20382677|clash_active=0|drop_lavacheck_faster=1|isPayingUser=1|usingStoreNavigation=1|enableInventoryTab=1|bigBackpack=1",
            0u
        });
    });
}