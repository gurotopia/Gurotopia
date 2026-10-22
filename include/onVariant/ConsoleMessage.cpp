#include "pch.hpp"

#include "ConsoleMessage.hpp"

void on::ConsoleMessage(ENetPeer *peer, const std::string &message, signed limit, int delay)
{
    send_varlist(peer, { "OnConsoleMessage", message, limit }, -1, delay);
}