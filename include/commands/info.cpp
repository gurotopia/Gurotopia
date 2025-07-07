#include "pch.hpp"
#include "action/wrench.hpp"
#include "info.hpp"

void info(ENetEvent& event, const std::string_view text)
{
    std::string name{ text.substr(sizeof("info ")-1) };
    peers(event, PEER_ALL, [&event, name](ENetPeer& p) 
    {
        /* @todo handle peer's offline or not in a world (no netid) */
        if (_peer[&p]->ltoken[0] == name)
        {
            action::wrench(event, std::format("action|wrench|\n|netid|{}", _peer[&p]->netid)); // @note imitate action|wrench
            return; // @note early exit
        }
    });
} 