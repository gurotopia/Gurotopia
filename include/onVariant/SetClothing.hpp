#pragma once

namespace on
{
    extern void SetClothing(ENetPeer &peer);
    extern void SetClothing(ENetPeer &to, ::peer &who);
    extern void SetClothing(ENetPeer &to, int netid, u_int skin, int state = 0, const std::array<float, 10ull> *cloth = nullptr); // @note for bots - no peer needed
}
