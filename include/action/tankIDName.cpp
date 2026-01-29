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

    packet::create(*event.peer, false, 0, { "OnOverrideGDPRFromServer", 18, 1, 0, 1 });
    packet::create(*event.peer, false, 0, { "OnSetRoleSkinsAndTitles", "000000", "000000" });

    /* v5.40 */
    packet::create(*event.peer, false, 0, {
        "OnSuperMainStartAcceptLogonHrdxs47254722215a",
        2816436900u, // @note items.dat
        "ubistatic-a.akamaihd.net",
        "0098/0290120267/cache/",
        "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro com.proziro.zacro com.slash.gamebuster",
        "proto=225|choosemusic=audio/mp3/about_theme.mp3|active_holiday=0|wing_week_day=0|ubi_week_day=0|server_tick=33784663|game_theme=|clash_active=1|drop_lavacheck_faster=1|isPayingUser=1|usingStoreNavigation=1|enableInventoryTab=1|bigBackpack=1|seed_diary_hash=3701384193|m_clientBits=|eventButtons={\"EventButtonData\":[{\"active\":false,\"buttonAction\":\"eventmenu\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":6244,\"name\":\"ClashEventButton\",\"order\":9,\"rcssClass\":\"clash-event\",\"text\":\"\"},{\"active\":true,\"buttonAction\":\"dailychallengemenu\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":23,\"name\":\"DailyChallenge\",\"order\":10,\"rcssClass\":\"daily_challenge\",\"text\":\"\"},{\"active\":true,\"buttonAction\":\"openPiggyBank\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"PiggyBankButton\",\"order\":20,\"rcssClass\":\"piggybank\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"showdungeonsui\",\"buttonTemplate\":\"DungeonEventButton\",\"counter\":0,\"counterMax\":20,\"name\":\"ScrollsPurchaseButton\",\"order\":30,\"rcssClass\":\"scrollbank\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"show_mailbox_ui\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"MailboxButton\",\"order\":30,\"rcssClass\":\"mailbox\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"show_auction_ui\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"AuctionButton\",\"order\":30,\"rcssClass\":\"auction\",\"text\":\"\"},{\"active\":false,\"buttonTemplate\":\"ActiveAuctionEventButton\",\"counter\":0,\"counterMax\":20,\"name\":\"ActiveAuctionButton\",\"order\":30,\"rcssClass\":\"activeauction\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"eventmenu\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":6244,\"name\":\"ClashEventButton\",\"order\":21,\"rcssClass\":\"clash-event\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"show_bingo_ui\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"WinterBingoButton\",\"order\":49,\"rcssClass\":\"wf-bingo\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"show_bingo_ui\",\"buttonTemplate\":\"BaseEventButton\",\"name\":\"UbiBingoButton\",\"order\":50,\"rcssClass\":\"ubi-bingo\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"winterrallymenu\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"WinterRallyButton\",\"order\":50,\"rcssClass\":\"winter-rally\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"leaderboardBtnClicked\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"AnniversaryLeaderboardButton\",\"order\":50,\"rcssClass\":\"anniversary-leaderboard\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"euphoriaBtnClicked\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"AnniversaryEuphoriaButton\",\"order\":50,\"rcssClass\":\"anniversary-euphoria\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"openLnySparksPopup\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":5,\"name\":\"LnyButton\",\"order\":50,\"rcssClass\":\"cny\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"ShowValentinesQuestDialog\",\"buttonTemplate\":\"EventButtonWithCounter\",\"counter\":0,\"counterMax\":100,\"name\":\"ValentinesButton\",\"order\":50,\"rcssClass\":\"valentines_day\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"showegseeventui\",\"buttonTemplate\":\"EventButtonWithCounter\",\"counter\":0,\"counterMax\":20,\"name\":\"EasterButton\",\"order\":50,\"rcssClass\":\"easter_event\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"openStPatrickPiggyBank\",\"buttonTemplate\":\"BaseEventButton\",\"name\":\"StPatrickPBButton\",\"order\":50,\"rcssClass\":\"st_patrick_event\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"dailyrewardmenu\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":1,\"name\":\"CincoPinataButton\",\"order\":50,\"rcssClass\":\"cinco_pinata_event\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"show_fruit_mixer_dialog\",\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"name\":\"SPP_TropicalFruitsButton\",\"order\":50,\"rcssClass\":\"spp_tropical_fruits\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"claimprogressbar\",\"buttonTemplate\":\"EventButtonWithCounter\",\"counter\":0,\"counterMax\":0,\"name\":\"SummerfestButton\",\"order\":50,\"rcssClass\":\"summerfest\",\"text\":\"\"},{\"active\":false,\"buttonAction\":\"claimprogressbar\",\"buttonTemplate\":\"EventButtonWithCounter\",\"counter\":0,\"counterMax\":15,\"name\":\"HalloweenButton\",\"order\":50,\"rcssClass\":\"halloween\",\"text\":\"\"}]}"
    });

    packet::create(*event.peer, false, 0, { "OnEventButtonDataSet", "ClashEventButton", 1, "{\"active\":true,\"buttonAction\":\"eventmenu\",\"buttonState\":0,\"buttonTemplate\":\"BaseEventButton\",\"counter\":0,\"counterMax\":0,\"itemIdIcon\":6828,\"name\":\"ClashEventButton\",\"notification\":0,\"order\":9,\"rcssClass\":\"clash-event\",\"text\":\"Claim!\"}" });
}