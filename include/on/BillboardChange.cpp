#include "pch.hpp"
#include "tools/string.hpp"
#include "BillboardChange.hpp"

void on::BillboardChange(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    packet::create(*event.peer, true, 0, {
        "OnBillboardChange",
        pPeer->netid,
        signed{pPeer->billboard.id},
        std::format("{},{}", to_char(pPeer->billboard.show), to_char(pPeer->billboard.isBuying)).c_str(),
        pPeer->billboard.price,
        signed{pPeer->billboard.perItem}
    });
}