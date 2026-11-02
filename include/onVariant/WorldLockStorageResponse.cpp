#include "pch.hpp"

#include "WorldLockStorageResponse.hpp"

void on::WorldLockStorageResponse(ENetPeer *peer) 
{
    ::peer *pPeer = static_cast<::peer*>(peer->data);

    const u_int count = pPeer->wl_storage;
    std::string data{"\x95\x90", 2}; // @note msgpack [[], balance, 0, 0, 0]
    struct encoding { u_int maximum; u_char prefix; int bytes; };
    constexpr encoding formats[]{
        {0x7f, 0, 1}, {0xff, 0xcc, 1}, {0xffff, 0xcd, 2}, {0xffffffffu, 0xce, 4}
    };
    const encoding *const format = std::ranges::find_if(formats, [count](const encoding &entry) { return count <= entry.maximum; });
    if (format->prefix != 0) data.push_back(static_cast<char>(format->prefix));
    for (int i = format->bytes - 1; i >= 0; --i) data.push_back(static_cast<char>(count >> (i * 8)));
    data.append(3, '\0');

    send_varlist(peer, { "OnWorldLockStorageResponse", data });
}
