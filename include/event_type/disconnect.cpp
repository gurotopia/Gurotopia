#include "pch.hpp"
#include "action/quit.hpp"
#include "disconnect.hpp"

void disconnect(ENetEvent& event)
{
    action::quit(event, "");
}
