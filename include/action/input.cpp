#include "pch.hpp"
#include "commands/__command.hpp"
#include "on/ConsoleMessage.hpp"
#include "input.hpp"

using namespace std::chrono;

void action::input(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::hPipe hPipe{ header };
    std::string text = hPipe["text"];

    if (text.empty() || text.front() == '\r' || std::ranges::all_of(text, ::isspace)) return;
    text.erase(text.begin(), std::find_if_not(text.begin(), text.end(), ::isspace));
    text.erase(std::find_if_not(text.rbegin(), text.rend(), ::isspace).base(), text.end());
    if (text.empty()) return; // @note we recheck if empty since we did trimming.
    
    steady_clock::time_point now = steady_clock::now();
    pPeer->messages.push_back(now);
    if (pPeer->messages.size() > 5) pPeer->messages.pop_front();
    if (pPeer->messages.size() == 5 && duration_cast<std::chrono::seconds>(now - pPeer->messages.front()).count() < 6)
    {
        on::ConsoleMessage(event.peer,
            "`6>>`4Spam detected! ``Please wait a bit before typing anything else.  "  
            "Please note, any form of bot/macro/auto-paste will get all your accounts banned, so don't do it!");
    }
    else if (text.starts_with('/')) 
    {
        send_action(*event.peer, "log", std::format("msg| `6{}``", text));
        std::string command = text.substr(1, text.find(' ') - 1);
        
        if (auto it = cmd_pool.find(command); it != cmd_pool.end()) 
            it->second(std::ref(event), std::move(text.substr(1)/* remove the '/' */));
        else 
            send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
    }
    else 
    {
        if (pPeer->state & S_DUCT_TAPE)
        {
            static constexpr std::array<std::string_view, 4zu> muffled{ "mfmm", "mmfmfm", "mffm", "mfmfmm" };

            std::string muffled_text{};
            muffled_text.reserve(text.size());

            std::size_t word_index{};
            for (std::size_t i = 0zu; i < text.size();)
            {
                if (std::isspace(text[i]))
                {
                    muffled_text += text[i++];
                    continue;
                }

                while (i < text.size() && !std::isspace(text[i]))
                    ++i;

                muffled_text += muffled[word_index % muffled.size()];
                ++word_index;
            }
            text = std::move(muffled_text);
        }
        const std::string &player_chat = std::format("CP:0_PL:0_OID:_player_chat={}", text);
        const std::string &message = std::format("CP:0_PL:0_OID:_CT:[W]_ `6<`{}{}``>`` `$`${}````", pPeer->prefix, pPeer->growid, text);
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer, player_chat, message](ENetPeer& p) 
        {
            send_varlist(&p, { "OnTalkBubble", pPeer->netid, player_chat });
            on::ConsoleMessage(&p, message);
        });
    }
}
