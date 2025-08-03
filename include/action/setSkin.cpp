#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "tools/string.hpp"
#include "setSkin.hpp"

void action::setSkin(ENetEvent& event, const std::string& header)
{
    std::vector<std::string> pipes = readch(std::move(header), '|');

    _peer[event.peer]->skin_color = stoul(pipes[3zu]); // @todo handle non-numrials
    on::SetClothing(event);
}