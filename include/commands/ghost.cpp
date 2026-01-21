#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "ghost.hpp"

void ghost(ENetEvent& event, const std::string_view text)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    peer->state ^= S_GHOST;

    on::SetClothing(*event.peer);
}