#include "pch.hpp"
#include <ctime>
#include "SetBux.hpp"

void on::SetBux(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->gems = std::clamp<long long>(pPeer->gems, 0LL, std::numeric_limits<long long>::max());

    if (pPeer->game_version >= 5.61f)
    {
        const float seconds_from_midnight = static_cast<float>(std::time(nullptr) % 86400);
        send_varlist(event.peer, {
            "OnSetBux",
            static_cast<long long>(pPeer->gems),
            1,
            0,
            CL_Vec3f{ seconds_from_midnight, 0.0f, 0.0f }
        });
    }
    else
    {
        const int legacy_gems = static_cast<int>(std::min<long long>(pPeer->gems, std::numeric_limits<int>::max()));
        send_varlist(event.peer, { "OnSetBux", legacy_gems, 1, 1 });
    }
}
