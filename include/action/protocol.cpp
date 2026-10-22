#include "pch.hpp"
#include "https/server_data.hpp"
#include "proton/Variant.hpp"

#include "protocol.hpp"

void action::protocol(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    bool is_reg = false;
    try
    {
        std::vector<std::string> pipes = readch(header, '|');
        if (pipes.size() < 4ull) throw std::runtime_error("");

        if (pipes[2ull] == "ltoken")
        {
            const std::string &decoded = base64_decode(pipes[3ull]);
            if (decoded.empty()) throw std::runtime_error("");

            if (std::size_t pos = decoded.find("growId="); pos != std::string::npos)
            {
                pos += sizeof("growId=") - 1;
                pPeer->growid = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos)
            {
                pos += sizeof("password=") - 1;
                pPeer->password = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("reg="); pos != std::string::npos)
            {
                pos += sizeof("reg=") - 1;
                std::string reg_str = decoded.substr(pos, decoded.find('&', pos) - pos);
                is_reg = (reg_str == "1");
            }
        } // @note delete decoded
        if (pPeer->growid.empty() || pPeer->password.empty()) throw std::runtime_error("");
    }
    catch (...) {
        send_action(*event.peer, "logon_fail", "");
        return; // @note stop processing invalid protocol data
    }

    bool loaded = false;
    if (is_reg)
    {
        loaded = pPeer->create_account(pPeer->growid, pPeer->password);
        if (!loaded)
        {
            send_action(*event.peer, "log", "msg|`4Registration failed:`` GrowID already exists.");
            send_action(*event.peer, "logon_fail", "");
            enet_peer_disconnect_later(event.peer, 0);
            return;
        }
    }
    else
    {
        loaded = pPeer->load(pPeer->growid, pPeer->password);
        if (!loaded)
        {
            send_action(*event.peer, "log", "msg|`4Logon failed:`` Invalid GrowID or password.");
            send_action(*event.peer, "logon_fail", "");
            enet_peer_disconnect_later(event.peer, 0);
            return;
        }
    }

    send_varlist(event.peer, {
        "OnSendToServer", 
        (signed)gServer_data.port, 
        0, 
        pPeer->user_id, 
        std::format("{}|0|0", gServer_data.server), // @todo UUIDToken
        1, 
        pPeer->growid.c_str()
    }); // @note PACKET_DISCONNECT from client.
}