#include "pch.hpp"

#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    if (pipes[10] == "checkbox_public" && pipes[11] == "1"/*true*/ || pipes[11] == "0"/*false*/)
    {
        auto it = worlds.find(peer->recent_worlds.back());
        if (it == worlds.end()) return;

        it->second._public = atoi(pipes[11].c_str());

        // @todo add public lock visuals
    }
}