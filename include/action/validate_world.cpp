#include "pch.hpp"

#include "validate_world.hpp"

void action::validate_world(ENetEvent& event, const std::string& header)
{
    const std::string name = ::hPipe{header}["name"];
    if (name.length() > 24 || !alnum(name)) return;
    send_action(event.peer, "world_validated", std::format("available|0\nworld_name|{}", name));
}
