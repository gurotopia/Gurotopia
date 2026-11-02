#include "pch.hpp"

#include "msg.hpp"

void msg(ENetEvent& event, const std::string_view text)
{
    const std::string message{ text.substr(sizeof("me ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> spaces = readch(message, ' ');
    std::string growid = spaces[0];
    std::string msg;
    for (std::size_t i = 1/*@note skip growid*/; i < spaces.size(); ++i)
    {
        msg += spaces[i];
        if (i < spaces.size() - 1) msg += " ";
    }
    send_gameMessage(event.peer, {.message = std::format("action|log\nmsg|CT:[MSG]_ `6/msg {} {}``", growid, msg)});

    bool online = false;
    peers("", PeerSearch::EVERYONE, [&event, &pPeer, &online, growid, msg](ENetPeer *peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer->data);
        if (growid == pPeer->growid) // @todo add ignore
        {
            send_gameMessage(event.peer, {.message = "action|log\n`6>> `4SYSTEM``: Sorry! This player has ignored private messages from non-friends. Become friends to chat!"});
            online = true;
        }
        else if (pOthers->growid == growid)
        {
            send_gameMessage(event.peer, {.message = std::format("action|log\nmsg|CT:[MSG]_`6>> (Sent to `${}`)``", growid, msg)});
            online = true;
        }
        if (online) return;
    });
    if (!online)
    {
        std::string hint = (growid.length() < 5) ? " (you need to enter at least 5 letters if you meant a longer name!)" : "";
        send_gameMessage(event.peer, {.message = std::format("action|log\nmsg|`6>> `{}` is not online right now{}.``", growid, hint)});
    }
}
