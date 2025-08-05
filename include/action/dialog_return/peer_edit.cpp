#include "pch.hpp"

#include "on/NameChanged.hpp" // @note peer name changes when role updates
#include "on/SetBux.hpp" // @note update gem count @todo if changed. (currently it updates everytime peer is edited)

#include "peer_edit.hpp"

void peer_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const std::string name = pipes[5zu];
    const bool status = atoi(pipes[8zu].c_str());

    const u_char role = atoi(pipes[11zu].c_str());
    const short level = atoi(pipes[13zu].c_str());
    const signed gems = atoi(pipes[15zu].c_str());

    if (status) // @note online
    {
        peers(event, PEER_ALL, [&event, name, role, level, gems](ENetPeer& p) 
        {
            if (_peer[&p]->ltoken[0] == name)
            {
                auto &_p = _peer[&p];
                _p->role = role;
                _p->level[0] = level;
                _p->gems = gems;

                on::NameChanged(event);
                on::SetBux(event);
                return;
            }
        });
    }
    else // @note offline
    {
        ::peer &offline = ::peer().read(name);
        offline.role = role;
        offline.level[0] = level;
        offline.gems = gems;
    }
}