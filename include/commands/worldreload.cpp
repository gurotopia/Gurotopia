#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "worldreload.hpp"

void reload_world_all(const std::string &name)
{
    peers(name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ENetEvent fake{ .peer = &p };
        action::quit_to_exit(fake, "", true);
        action::join_request(fake, "", name);
    });
}
