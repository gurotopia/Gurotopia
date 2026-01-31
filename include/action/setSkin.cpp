#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "tools/string.hpp"
#include "setSkin.hpp"

void action::setSkin(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(header, '|');
    if (pipes.size() < 4zu) return;

    ::peer *peer = static_cast<::peer*>(event.peer->data);
    try
    {
        peer->skin_color = stol(pipes[3]);
        on::SetClothing(*event.peer);
    }
    catch (const std::invalid_argument &ex)
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`4Invalid input. ``id must be a `wnumber``."
        });
    }
    catch (const std::out_of_range &ex)
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`4Invalid input. ``id is out of range."
        });
    }
}
