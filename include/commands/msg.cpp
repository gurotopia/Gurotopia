#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "lockaccess.hpp"
#include "msg.hpp"

static void deliver(ENetEvent& event, const std::string &target, const std::string &body)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string lower_target = target;
    for (char &c : lower_target) c = std::tolower(static_cast<unsigned char>(c));

    bool found = false;
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget || pTarget->growid.empty()) return;

        std::string their_name = pTarget->growid;
        for (char &c : their_name) c = std::tolower(static_cast<unsigned char>(c));
        if (their_name != lower_target) return;
        if (pTarget->user_id == pPeer->user_id) return;

        found = true;
        pTarget->last_msg_from = pPeer->growid;

        on::ConsoleMessage(&p, std::format("`6[from {}`6]`` `o{}", chat_name(*pPeer), body));
        on::ConsoleMessage(event.peer, std::format("`6[to {}`6]`` `o{}", chat_name(*pTarget), body));
    });

    if (!found)
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not online.``", target));
}

/* /msg {player} {message} */
void msg(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    std::string target{}, body{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        target = arg.substr(0, sp);
        body = arg.substr(sp + 1);
    }

    while (!body.empty() && std::isspace(static_cast<unsigned char>(body.back()))) body.pop_back();

    if (target.empty() || body.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /msg {player} {message}``");
        return;
    }

    deliver(event, target, body);
}

/* /reply {message} -> answers whoever last messaged you */
void reply(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->last_msg_from.empty())
    {
        on::ConsoleMessage(event.peer, "`4Nobody has messaged you yet.``");
        return;
    }

    std::string body{ text };
    if (const std::size_t sp = body.find(' '); sp != std::string::npos) body = body.substr(sp + 1);
    else body.clear();

    while (!body.empty() && std::isspace(static_cast<unsigned char>(body.back()))) body.pop_back();

    if (body.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /reply {message}``");
        return;
    }

    deliver(event, pPeer->last_msg_from, body);
}
