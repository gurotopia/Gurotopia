#include "pch.hpp"
#include "BillboardChange.hpp"

void on::BillboardChange(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    send_varlist(event.peer, {
        "OnBillboardChange",
        pPeer->netid,
        signed{pPeer->billboard.id},
        std::format("{},{}", to_char(pPeer->billboard.show), to_char(pPeer->billboard.isBuying)),
        pPeer->billboard.price,
        signed{pPeer->billboard.perItem}
    });
}