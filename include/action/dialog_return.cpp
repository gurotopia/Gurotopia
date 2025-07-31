#include "pch.hpp"

#include "tools/string.hpp"
#include "dialog_return/__dialog_return.hpp"

#include "action/dialog_return.hpp"

void action::dialog_return(ENetEvent& event, const std::string& header) 
{
    auto &peer = _peer[event.peer];
    std::vector<std::string> pipes = readch(std::move(header), '|');

    if (pipes.size() <= 3zu) return; // if button has no name or has no field.

    // @note found at ./action/dialog_return/
    if (auto it = dialog_return_pool.find(pipes[3zu]); it != dialog_return_pool.end()) 
        it->second(std::ref(event), std::move(pipes));
}