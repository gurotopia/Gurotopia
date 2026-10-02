#include "pch.hpp"
#include "action/__action.hpp"
#include "gamePacket/_gamePacket.hpp"
#include "receive.hpp"

void receive(ENetEvent& event) 
{
    std::span<const enet_uint8> data{event.packet->data, event.packet->dataLength};
    switch (data[0ull]) 
    {
        case 2: case 3: 
        {
            std::string header{data.begin() + 4, data.end() - 1};
            if (header.find("ltoken|") == std::string::npos && header.find("password") == std::string::npos) puts(header.c_str()); // @note never print logins (they contain passwords)
            
            std::ranges::replace(header, '\n', '|');
            const std::vector<std::string> pipes = readch(header, '|');
            if (pipes.size() < 2) break;
            
            std::string action{};
            if (pipes[0ull] == "protocol" || pipes[0ull] == "tankIDName")
            {
                action = pipes[0ull];
            }
            else action = std::format("{}|{}", pipes[0ull], pipes[1ull]);

            if (const auto i = action_pool.find(action); i != action_pool.end())
                i->second(event, header);
            break;
        }
        case 4: 
        {
            if (event.packet->dataLength < sizeof(::gamePacket)) break;

            ::gamePacket gamePacket = make_gamePacket(event.packet->data);
            gamePacket.size = event.packet->dataLength - sizeof(::gamePacket); // @todo did i do this right? or check for flag ::EXTENDED

            if (const auto i = gamePacket_pool.find(gamePacket.type); i != gamePacket_pool.end())
                i->second(event, std::move(gamePacket));
            break;
        }
    }
    enet_packet_destroy(event.packet);
}

