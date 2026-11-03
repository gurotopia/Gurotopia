#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"

#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    std::vector<std::string> spaces = readch(text.data(), ' ');
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string message = (spaces.size() > 1) ? spaces[1] : "";
    u_int cost = host->connectedPeers * 30;

    if (message.empty())
    {
        send_gameMessage(event.peer, {.message = std::format(
            "action|log\nmsg|`5msg|>> This lets you send a message to {}`` random users online and make an annoying beep noise.  Costs `${} Gems`` to use. Example:`` `w/sb Hey guys, come to my world!",
            host->connectedPeers, cost)
        });
    }
    else {
        std::string display = pPeer->recent_worlds.back();
        peers("", PeerSearch::EVERYONE, [&pPeer, message, display](ENetPeer *peer)
        {
            on::ConsoleMessage(peer, 
                std::format(
                    "CP:0_PL:0_OID:_CT:[SB]_ `5** from ({}```5) in [```${}```5] ** : ```${}``",
                    pPeer->display_growid, display, message
                )
            );
            send_gameMessage(peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});
        });
    }
}
