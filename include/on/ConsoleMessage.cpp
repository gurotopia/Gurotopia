#include "pch.hpp"

#include "ConsoleMessage.hpp"

void on::ConsoleMessage(ENetPeer *peer, const std::string &message, int delay)
{
    send_varlist(peer, { "OnConsoleMessage", message });
}