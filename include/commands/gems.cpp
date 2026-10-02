#include "pch.hpp"
#include "onVariant/SetBux.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "gems.hpp"

void gems(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (text.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /gems {amount}``");
        return;
    }

    signed amount{};
    try
    {
        std::string arg{ text };
        std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)) && c != '-'; });
        amount = std::stoi(arg);
    }
    catch (...)
    {
        on::ConsoleMessage(event.peer, "`4That is not a valid number.``");
        return;
    }

    pPeer->gems = amount;
    on::SetBux(event);
    on::ConsoleMessage(event.peer, std::format("`2Gems set to `w{}``.``", pPeer->gems));
}
