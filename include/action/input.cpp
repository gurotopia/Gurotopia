#include "pch.hpp"
#include "commands/__command.hpp"
#include "tools/string.hpp"
#include "input.hpp"

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif

void action::input(ENetEvent& event, const std::string& header)
{
    auto &peer = _peer[event.peer];

    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.size() < 5) return;
    if (pipes[3] != "text") return;

    std::string text = pipes[4];

    if (text.front() == '\r' || std::ranges::all_of(text, ::isspace)) return;
    text.erase(text.begin(), std::find_if_not(text.begin(), text.end(), ::isspace));
    text.erase(std::find_if_not(text.rbegin(), text.rend(), ::isspace).base(), text.end());
    
    steady_clock::time_point now = steady_clock::now();
    peer->messages.push_back(now);
    if (peer->messages.size() > 5) peer->messages.pop_front();
    if (peer->messages.size() == 5 && duration_cast<std::chrono::seconds>(now - peer->messages.front()).count() < 6)
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage", 
            "`6>>`4Spam detected! ``Please wait a bit before typing anything else.  "  
            "Please note, any form of bot/macro/auto-paste will get all your accounts banned, so don't do it!"
        });
    else if (text.starts_with('/')) 
    {
        packet::action(*event.peer, "log", std::format("msg| `6{}``", text));
        std::string command = text.substr(1, text.find(' ') - 1);
        
        if (auto it = cmd_pool.find(command); it != cmd_pool.end()) 
            it->second(std::ref(event), std::move(text.substr(1)));
        else 
            packet::action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
    }
    else peers(event, PEER_SAME_WORLD, [&peer, &text](ENetPeer& p) 
    {
        if (peer->ducttape) text = "mfmm"; // @todo scalewith length of message. e.g. "hello" -> "mfmm"; "hello world" -> "mfmm mmfmfm"
        packet::create(p, false, 0, {
            "OnTalkBubble", 
            peer->netid, 
            std::format("CP:0_PL:0_OID:_player_chat={}", text).c_str()
        });
        packet::create(p, false, 0, {
            "OnConsoleMessage", 
            std::format("CP:0_PL:0_OID:_CT:[W]_ `6<`{}{}``>`` `$`${}````", 
                peer->prefix, peer->ltoken[0], text).c_str()
        });
    });
}
