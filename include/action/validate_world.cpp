#include "pch.hpp"

#include "validate_world.hpp"

void action::validate_world(ENetEvent& event, const std::string& header)
{
    const std::string name = ::hPipe{header}["name"];
    char available = 0;
    if (name.length() > 24 || !isalnum(name)) available = -1;

    send_gameMessage(event.peer, {.message = std::format("world_name|{}\naction|world_validated\navailable|{}", name, available)});
}
