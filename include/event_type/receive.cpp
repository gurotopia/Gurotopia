#include "pch.hpp"
#include "action/__action.hpp"
#include "state/__states.hpp"
#include "receive.hpp"

#include "tools/string_view.hpp"

void receive(ENetEvent event) 
{
    std::span<enet_uint8> data{event.packet->data, event.packet->dataLength};
    switch (data[0zu]) 
    {
        case 2: case 3: 
        {
            std::string header{data.begin() + 4, data.end() - 1};
#if defined(_MSC_VER)
            printf("%s\n", header.c_str());
#else
            printf("\e[38;5;249m%s\e[0m\n", header.c_str());
#endif
            std::ranges::replace(header, '\n', '|');
            std::vector<std::string> pipes = readch(header, '|');
            
            const std::string action = (pipes[0zu] == "protocol") ? pipes[0zu] : std::format("{}|{}", pipes[0zu], pipes[1zu]);
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
                    if ((size + 4zu) >= 60zu) {
                        std::byte *_1bit = reinterpret_cast<std::byte*>(event.packet->data) + 4;
                        for (std::size_t i = 0zu; i < size; ++i)
                            raw_state[i] = _1bit[i];
                    }
                    if ((std::to_integer<bool>(raw_state[12] & std::byte{ 0x08 })) && 
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