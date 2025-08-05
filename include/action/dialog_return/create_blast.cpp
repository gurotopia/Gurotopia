#include "pch.hpp"

#include "action/quit_to_exit.hpp" // @note peer leave world
#include "action/join_request.hpp" // @note peer enter (blast) world

#include "create_blast.hpp"

void create_blast(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const u_short id = atoi(pipes[5zu].c_str());
    std::string world_name = pipes[8zu];
    
    std::for_each(world_name.begin(), world_name.end(), [](char& c) { c = std::toupper(c); });

    switch (id)
    {
        case 1402: // @note Thermonuclear Blast
        {
            auto [it, inserted] = worlds.try_emplace(world_name, world_name);
            world &world = it->second;
            if (world.name.empty())
            {
                action::quit_to_exit(event, "", true);
                blast::thermonuclear(world, world_name);
                action::join_request(event, "", world_name);
            }
            break;
        }
    }
}