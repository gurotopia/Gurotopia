#include "pch.hpp"
#include "tools/string.hpp"

#include "tankIDName.hpp"

void action::tankIDName(ENetEvent& event, const std::string& header)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.empty() || pipes.size() < 41zu) enet_peer_disconnect_later(event.peer, 0);

    for (std::size_t i = 0; i < pipes.size(); ++i) 
    {
        if      (pipes[i] == "tankIDName")   peer->ltoken[0] = pipes[i+1];
        else if (pipes[i] == "game_version") peer->game_version = pipes[i+1];
        else if (pipes[i] == "country")      peer->country = pipes[i+1];
        else if (pipes[i] == "user")         peer->user_id = std::stoi(pipes[i+1]); // @todo validate user_id
    }
    peer->read(peer->ltoken[0]);

    /* v5.40 */
    packet::create(*event.peer, false, 0, {
        "OnSuperMainStartAcceptLogonHrdxs47254722215a",
        2816436900u, // @note items.dat
        "ubistatic-a.akamaihd.net",
        "0098/070120258/cache/",
        "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro com.proziro.zacro com.slash.gamebuster",
        "proto=225|choosemusic=audio/mp3/about_theme.mp3|active_holiday=0|wing_week_day=0|ubi_week_day=0|server_tick=69027239|game_theme=|clash_active=0|drop_lavacheck_faster=1|isPayingUser=1|usingStoreNavigation=1|enableInventoryTab=1|bigBackpack=1|seed_diary_hash=120408577",
        0u // @note player_tribute.dat
    });
}