#include "pch.hpp"

#include "on/BillboardChange.hpp"

#include "billboard_edit.hpp"

void billboard_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    for (std::size_t i = 0; i < pipes.size(); ++i) 
    {
        if (pipes[i] == "billboard_item")          
        {
            const short id = stoi(pipes[i+1]);

            if (id == 18 || id == 32) return; // @note list of items that cannot be put on billboard.
            
            pPeer->billboard.id = id;
        }
        else if (pipes[i] == "billboard_toggle")        pPeer->billboard.show = stoi(pipes[i+1]);
        else if (pipes[i] == "billboard_buying_toggle") pPeer->billboard.isBuying = stoi(pipes[i+1]);
        else if (pipes[i] == "chk_peritem")             pPeer->billboard.perItem = stoi(pipes[i+1]);
        else if (pipes[i] == "setprice")                pPeer->billboard.price =  stoi(pipes[i+1]);
    }
    on::BillboardChange(event);
}