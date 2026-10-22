#include "pch.hpp"
#include <ctime>
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->gems = std::clamp(pPeer->gems, 0, std::numeric_limits<signed>::max());

    if (pPeer->game_version >= 5.61f)
    {
        const float seconds_from_midnight = static_cast<float>(std::time(nullptr) % 86400);
        send_varlist(event.peer, {
            "OnSetBux",
            static_cast<int64_t>(pPeer->gems),
            1,
            0,
            CL_Vec3f{ seconds_from_midnight, 0.0f, 0.0f }
        });
    }
    else
    {
        send_varlist(event.peer, { "OnSetBux", pPeer->gems, 1, 1 });
    }
}
