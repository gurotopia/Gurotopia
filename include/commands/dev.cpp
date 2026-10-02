#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/NameChanged.hpp"
#include "dev.hpp"

/* /dev {password} -> grants yourself developer, regardless of current role.
 * change DEV_PASSWORD below to whatever you like. */
static constexpr std::string_view DEV_PASSWORD = "change-me";

void dev(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    // trim stray whitespace / newlines the client may append
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg != DEV_PASSWORD)
    {
        send_action(*event.peer, "log", "msg|`4Unknown command.`` Enter `$/?`` for a list of valid commands.");
        return;
    }

    pPeer->role = DEVELOPER;

    if (pPeer->nickname.empty())
        pPeer->display_growid = std::format("`6@{}``", pPeer->growid);

    on::NameChanged(event);
    on::ConsoleMessage(event.peer, "`2You are now a `wDeveloper``.``");
}
