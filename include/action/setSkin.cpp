#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "tools/string.hpp"
#include "setSkin.hpp"

void action::setSkin(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.size() < 4zu) return;

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    try
    {
        pPeer->skin_color = (u_int)stoul(pipes[3]);
        on::SetClothing(*event.peer);
    }
    catch (const std::logic_error &le) {} // @note std::invalid_argument std::out_of_range
}
