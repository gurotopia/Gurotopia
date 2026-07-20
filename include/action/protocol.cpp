#include "pch.hpp"
#include "https/server_data.hpp"
#include "proton/Variant.hpp"

#include "protocol.hpp"

void action::protocol(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
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
                pos += strlen("growId=");
                pPeer->growid = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
            if (std::size_t pos = decoded.find("password="); pos != std::string::npos) 
            {
                pos += strlen("password=");
                pPeer->password = decoded.substr(pos, decoded.find('&', pos) - pos);
            }
        } // @note delete decoded
        if (pPeer->growid.empty() || pPeer->password.empty()) throw std::runtime_error("");
    }
    catch (...) { 
        send_action(*event.peer, "logon_fail", "");
        return; // @note stop processing invalid protocol data
    }

    if (!pPeer->exists(pPeer->growid))
    {
        pPeer->mysql_insert("growid", pPeer->growid);
        
        pPeer->mysql_update<std::string>("password", pPeer->password);
    }
    pPeer->mysql_select_all();

    send_varlist(event.peer, {
        "OnSendToServer", 
        (int)gServer_data.port, 
        0, 
        pPeer->user_id, 
        std::format("{}|0|0", gServer_data.server), 
        1, 
        pPeer->growid.c_str() // @todo idk why this is 1028 if std::string
    });
}