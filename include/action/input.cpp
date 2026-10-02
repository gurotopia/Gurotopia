#include "pch.hpp"
#include "tools/bubble.hpp"
#include "commands/__command.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/time.hpp"
#include "commands/devmenu.hpp"
#include "input.hpp"

void action::input(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::hPipe hPipe{ header };
    std::string text = hPipe["text"];

    if (text.empty() || text.front() == '\r' || std::ranges::all_of(text, ::isspace)) return;
    text.erase(text.begin(), std::find_if_not(text.begin(), text.end(), ::isspace));
    text.erase(std::find_if_not(text.rbegin(), text.rend(), ::isspace).base(), text.end());
    if (text.empty()) return; // @note we recheck if empty since we did trimming.
    
    u_int now = ticks();
    pPeer->messages.push_back(now);
    if (pPeer->messages.size() > 5) pPeer->messages.pop_front();
    if (pPeer->role < MODERATOR && pPeer->messages.size() == 5 && now - pPeer->messages.front() < 6)
    {
        on::ConsoleMessage(event.peer,
            "`6>>`4Spam detected! ``Please wait a bit before typing anything else.  "  
            "Please note, any form of bot/macro/auto-paste will get all your accounts banned, so don't do it!");
    }
    else if (text.starts_with('/')) 
    {
        send_action(*event.peer, "log", std::format("msg| `6{}``", text));
        std::string command = text.substr(1, text.find(' ') - 1);
        if (pPeer->role >= DEVELOPER && text.find(' ') == std::string::npos && devmenu_form(event, command, true)) return; // @note form popup

        static const std::unordered_map<std::string_view, u_char> min_role{
            {"ghost", MODERATOR}, {"weather", DEVELOPER}, {"punch", DEVELOPER}, {"heal", DEVELOPER}, {"undress", DEVELOPER}
        };
        if (auto r = min_role.find(command); r != min_role.end() && pPeer->role < r->second)
        {
            send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
            return;
        }

        if (pPeer->curse_until > std::time(nullptr) && (command == "sb" || command == "msg" || command == "reply"))
        {
            tell(event.peer, "`4You can't do that while cursed.``");
            return;
        }
        
        if (auto it = cmd_pool.find(command); it != cmd_pool.end()) 
        {
            if (std::ranges::find(cmd_requires_arg, command) != cmd_requires_arg.end() && text.length() <= command.length()+1)
            {
                send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
            }
            else {
                try { it->second(std::ref(event), std::move(text.substr(1))); }
                catch (const std::exception &e)
                {
                    send_action(*event.peer, "log", std::format("msg|`4Command failed:`` {}", e.what()));
                    printf("[cmd] /%s failed: %s\n", command.c_str(), e.what());
                }
            }
        }
        else 
        {
            send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
        }
    }
    else 
    {
        if ((pPeer->state & S_DUCT_TAPE) || pPeer->curse_until > std::time(nullptr))
        {
            static constexpr std::array<std::string_view, 4ull> muffled{ "mfmm", "mmfmfm", "mffm", "mfmfmm" };

            std::string muffled_text{};
            muffled_text.reserve(text.size());

            std::size_t word_index{};
            for (std::size_t i = 0ull; i < text.size();)
            {
                if (std::isspace(text[i]))
                {
                    muffled_text += text[i++];
                    continue;
                }

                const std::size_t start = i;
                while (i < text.size() && !std::isspace(text[i])) ++i;

                const char last = text[i - 1];
                const bool punct = (last == '?' || last == '!' || last == '.' || last == ',');
                const std::size_t n = std::clamp<std::size_t>(i - start - (punct ? 1 : 0), 2, 8);

                std::string word{ "m" };
                for (std::size_t k = 1; k < n; ++k) word += (std::rand() % 3 == 0) ? 'f' : 'm';
                muffled_text += word;
                if (punct) muffled_text += last;
                ++word_index;
            }
            text = std::move(muffled_text);
        }

        const char *chat_color = (pPeer->role >= DEVELOPER) ? "`5" : (pPeer->role >= MODERATOR) ? "`^" : "`w";
        const std::string &player_chat = std::format("CP:0_PL:0_OID:_player_chat={}{}``", chat_color, text);
        const std::string &message = std::format("CP:0_PL:0_OID:_CT:[W]_ `6<{}>`` {}{}``", pPeer->display_growid, chat_color, text);
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, &pPeer, player_chat, message](ENetPeer& p) 
        {
            send_varlist(&p, { "OnTalkBubble", pPeer->netid, player_chat, 0u });
            on::ConsoleMessage(&p, message);
        });
    }
}
