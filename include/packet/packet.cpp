#include "pch.hpp"
#include "packet.hpp"

void packet::action(ENetPeer& p, const std::string& action, const std::string& str) 
{
    std::string_view action_view = std::format("action|{}\n", action);
    std::vector<u_char> data(4zu + action_view.length() + str.length(), 0x00);
    data[0zu] = 0x03;
    {
        const u_char *_1bit = reinterpret_cast<const u_char*>(action_view.data());
        for (std::size_t i = 0zu; i < action_view.length(); ++i)
            data[4zu + i] = _1bit[i];
    }
    if (!str.empty())
    {
        const u_char *_1bit = reinterpret_cast<const u_char*>(str.data());
        for (std::size_t i = 0zu; i < str.length(); ++i)
            data[4zu + action_view.length() + i] = _1bit[i];
    }
    
    enet_peer_send(&p, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}