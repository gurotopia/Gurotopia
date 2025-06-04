#include "pch.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "commands/__command.hpp"
#include "input.hpp"

#include "tools/string_view.hpp"

void input(ENetEvent event, const std::string& header)
{
    auto& peer = _peer[event.peer];
    if (not create_rt(event, 1, 400)) return;
    std::string text{readch(std::string{header}, '|')[4]};
    {
        if (text.empty()) return;
        auto start = std::ranges::find_if(text, [](unsigned char c) { return !std::isspace(c); });
        auto end = std::ranges::find_if(text.rbegin(), text.rend(), [](unsigned char c) { return !std::isspace(c); }).base();
        if (start < end) text.assign(start, end);
    } // @note delete start, end

    peer->messages.push_back(std::chrono::steady_clock::now());
    if (peer->messages.size() > 5) peer->messages.pop_front();
    if (peer->messages.size() == 5 && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - peer->messages.front()).count() < 6)
        gt_packet(*event.peer, false, 0, {
            "OnConsoleMessage", 
            "`6>>`4Spam detected! ``Please wait a bit before typing anything else.  "  
            "Please note, any form of bot/macro/auto-paste will get all your accounts banned, so don't do it!"
        });
    else if (text.starts_with('/')) 
    {
        action(*event.peer, "log", std::format("msg| `6{}``", text));
        std::string_view text_view = text.substr(1, text.find(' ') - 1); // @note e.g. /warp {} -> warp. this excludes the arguement and slash.
        if (const auto it = cmd_pool.find(text_view); it != cmd_pool.end()) 
            it->second(std::ref(event), std::move(text.substr(1)));
        else 
            action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
    }
    else peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        gt_packet(p, false, 0, {
            "OnTalkBubble", 
            peer->netid, 
            std::format("CP:0_PL:0_OID:_player_chat={}", text).c_str()
        });
        gt_packet(p, false, 0, {
            "OnConsoleMessage", 
            std::format("CP:0_PL:0_OID:_CT:[W]_ `6<`{}{}``>`` `$`${}````", 
                peer->prefix, peer->ltoken[0], text).c_str()
        });
    });
}
