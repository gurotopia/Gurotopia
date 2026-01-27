#include "pch.hpp"
#include "tools/string.hpp" // @note base64_decode()

#include "protocol.hpp"

void action::protocol(ENetEvent& event, const std::string& header)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    try 
    {
        std::vector<std::string> pipes = readch(header, '|');
        if (pipes.size() < 4zu) throw std::runtime_error("");

        if (pipes[2zu] == "ltoken")
        {
            const std::string decoded = base64_decode(pipes[3zu]);
            if (decoded.empty()) throw std::runtime_error("");

            if (std::size_t pos = decoded.find("growId="); pos != std::string::npos) 
            {
                pos += sizeof("growId=")-1zu;
                peer->ltoken[0] = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
            {
                pos += sizeof("password=")-1zu;
                peer->ltoken[1] = decoded.substr(pos);
            }
        } // @note delete decoded
    }
    catch (...) { packet::action(*event.peer, "logon_fail", ""); }

    packet::create(*event.peer, false, 0, {
        "OnSendToServer",
        17091,
        8172597,
        fnv1a(peer->ltoken[0]),
        "127.0.0.1|0|0260DCEB9063AC540552C15E90E9E639",
        1,
        peer->ltoken[0].c_str()
    });
    enet_peer_disconnect_later(event.peer, 0);
}
