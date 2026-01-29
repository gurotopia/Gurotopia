#include "pch.hpp"
#include "tools/string.hpp" // @note base64_decode()
#include "https/server_data.hpp"

#include "protocol.hpp"

void action::protocol(ENetEvent& event, const std::string& header)
{
    std::string growid{}, password{};
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
                growid = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
            {
                pos += sizeof("password=")-1zu;
                password = decoded.substr(pos);
            }
        } // @note delete decoded
        if (growid.empty() || password.empty()) throw std::runtime_error("");
    }
    catch (...) { 
        packet::action(*event.peer, "logon_fail", ""); 
        throw;
    }

    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, growid.c_str(), ""}); // @todo temp fix, i will change later.

    packet::create(*event.peer, false, 0, {
        "OnSendToServer",
        (signed)g_server_data.port,
        8172597, // @todo
        (signed)fnv1a(growid), // @todo downsize to 4 bit
        std::format("{}|0|0260DCEB9063AC540552C15E90E9E639", g_server_data.server).c_str(),
        1,
        growid.c_str()
    });
    enet_peer_disconnect_later(event.peer, 0); // @note avoids this event.peer from lingering in the server.
}
