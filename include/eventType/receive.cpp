#include "pch.hpp"

#include "action/__action.hpp"
#include "gamePacket/_gamePacket.hpp"

#include "receive.hpp"

#include <algorithm>

void receive(ENetEvent& event) 
{
    const enet_uint8 *data = event.packet->data;
    const size_t dataLength = event.packet->dataLength;
    switch (data[0ull]) 
    {
        case 2: case 3: 
        {
            std::string header{ reinterpret_cast<const char*>(data + 4), dataLength - 5 };
            print(header);
            
            std::ranges::replace(header, '\n', '|');
            const std::vector<std::string> pipes = readch(header, '|');
            if (pipes.size() < 2) break;
            
            std::string action{};
            if (pipes[0ull] == "protocol" || pipes[0ull] == "tankIDName" || pipes[0ull] == "requestedName")
            {
                action = (pipes[0ull] == "requestedName") ? "tankIDName" : pipes[0ull];
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
        default: print(std::format("received unqiue: {}", data[0ull]));
    }
    enet_packet_destroy(event.packet);
}

