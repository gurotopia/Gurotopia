#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "action/__action.hpp"
#include "state/states"
#include "type_receive.hpp"

#include "tools/string_view.hpp"

void type_receive(ENetEvent event) 
{
    std::span<enet_uint8> data{event.packet->data, event.packet->dataLength};
    switch (data[0]) 
    {
        case 2: case 3: 
        {
            std::string header{data.begin() + 4, data.end() - 1};
            printf("\e[38;5;249m%s\e[0m\n", header.c_str());
            std::ranges::replace(header, '\n', '|');
            std::vector<std::string> pipes = readch(header, '|');
            const std::string action = (pipes[0] == "protocol") ? pipes[0] : std::format("{}|{}", pipes[0], pipes[1]);
            if (const auto i = action_pool.find(action); i != action_pool.end())
                i->second(event, header);
            break;
        }
        case 4: 
        {
            state state{};
            {
                std::vector<std::byte> raw_state{event.packet->dataLength - 4};
                {
                    std::size_t size = raw_state.size();
                    if ((size + 4) >= 60) {
                        std::byte *_1bit = reinterpret_cast<std::byte*>(event.packet->data) + 4;
                        for (std::size_t i = 0; i < size; ++i)
                            raw_state[i] = _1bit[i];
                    }
                    if ((raw_state[12] & std::byte{ 0x08 }) != std::byte{false} and 
                        size < static_cast<std::size_t>(*reinterpret_cast<int*>(&raw_state[52])) + 56) break;
                } // @note deletes size
                state = get_state(std::move(raw_state));
            } // @note deletes raw_state
            if (const auto i = state_pool.find(state.type); i != state_pool.end())
                i->second(event, std::move(state));
            break;
        }
    }
    enet_packet_destroy(event.packet);
}