#include "pch.hpp"

#include "msg.hpp"

void msg(ENetEvent& event, const std::string_view text)
{
    std::vector<std::string> spaces = readch(text.data(), ' ');
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string growid = (spaces.size() > 1) ? spaces[1] : "";
    std::string msg;
    for (std::size_t i = 2/*@note skip growid*/; i < spaces.size(); ++i)
    {
        msg += spaces[i];
        if (i < spaces.size() - 1) msg += " ";
    }
    send_gameMessage(event.peer, {.message = std::format("action|log\nmsg|CT:[MSG]_ `6/msg {} {}``", growid, msg)});

    if (growid.empty() || msg.empty())
    {
        send_gameMessage(event.peer, {.message = "action|log\nmsg|Usage: /msg <`$full or first part of a name``> <`$your message``> - This will send a private message to someone anywhere in the universe.  If you don't include a message, you can just see if he/she is online or not."});
    }
    else {
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
}
