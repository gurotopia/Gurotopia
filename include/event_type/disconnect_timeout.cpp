#include "pch.hpp"
#include "action/quit.hpp"
#include "disconnect_timeout.hpp"

void disconnect_timeout(ENetEvent& event)
{
    quit(event, "");
}