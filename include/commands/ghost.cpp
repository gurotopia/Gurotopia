#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "ghost.hpp"

void ghost(ENetEvent& event, const std::string_view text)
{
    auto &peer = _peer[event.peer];

    peer->state ^= S_GHOST;

    on::SetClothing(event);
}