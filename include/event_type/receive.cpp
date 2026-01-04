#include "pch.hpp"
#include "action/__action.hpp"
#include "state/__states.hpp"
#include "tools/string.hpp"
#include "receive.hpp"

void receive(ENetEvent& event) 
{
    std::span<const enet_uint8> data{event.packet->data, event.packet->dataLength};
    switch (data[0zu]) 
    {
        case 2: case 3: 
        {
            std::string header{data.begin() + 4, data.end() - 1};
            puts(header.c_str());
            
            std::ranges::replace(header, '\n', '|');
            const std::vector<std::string> pipes = readch(header, '|');
            
            const std::string action = (pipes[0zu] == "protocol") ? pipes[0zu] : std::format("{}|{}", pipes[0zu], pipes[1zu]);
            if (const auto i = action_pool.find(action); i != action_pool.end())
                i->second(event, header);
            break;
        }
        case 4: 
        {
            if (event.packet->dataLength < sizeof(::state)) return;

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(event.packet->data);
            ::state state = get_state({_1bit, _1bit + (event.packet->dataLength)});

            if (const auto i = state_pool.find(state.type); i != state_pool.end())
                i->second(event, std::move(state));
            break;
        }
    }
    enet_packet_destroy(event.packet);
}