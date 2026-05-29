#include "pch.hpp"

#include "tools/string.hpp"
#include "dialog_return/__dialog_return.hpp"

#include "action/dialog_return.hpp"

void action::dialog_return(ENetEvent& event, const std::string& header) 
{
    std::vector<std::string> pipes = readch(header, '|');

    if (pipes.size() <= 3zu) return; // if button has no name or has no field.

    // @note found at ./action/dialog_return/
    if (const auto it = dialog_return_pool.find(pipes[3zu]); it != dialog_return_pool.end()) 
        it->second(std::ref(event), std::move(pipes));
}