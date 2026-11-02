#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/WorldLockStorageResponse.hpp"
#include "onVariant/EventButtonDataSet.hpp"
#include "tools/conversions.hpp"

#include "worldlock_storage_getpage.hpp"
#include "worldlock_storage_modify_amount.hpp"

void action::worldlock_storage_modify_amount(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const ::hPipe hPipe{header};
    
    u_int amount = atoi(hPipe["amount"].c_str());
    if (amount <= 0) return;
    int type = atoi(hPipe["type"].c_str());
    if (type < -1 || type > 1) return;

    pPeer->wl_storage += type*amount;

    u_short wl = amount, 
            dl = amount / DIAMOND_LOCK_VALUE, 
            bgl = amount / BLUE_GEM_LOCK_VALUE;
    std::string str;

    if (type == 1)
    {
        wl = pPeer->have_slot({ 242, 0 }), 
        dl = pPeer->have_slot({ 1796, 0 }), 
        bgl = pPeer->have_slot({ 7188, 0 });
        convert_lock(amount, wl, dl, bgl, str);
    }
    else convert_lock(amount, wl, dl, bgl, str);

    if (bgl > 0) modify_item_inventory(event, ::slot(7188, -(type)*bgl));
    if (dl  > 0) modify_item_inventory(event, ::slot(1796, -(type)*dl));
    if (wl  > 0) modify_item_inventory(event, ::slot(242,  -(type)*wl));

    if (type == -1)
    {
        std::string message = std::format("You collected {}.", str);

        send_varlist(event.peer, { "OnTalkBubble", pPeer->netid, message, 0u, 1u });
        on::ConsoleMessage(event.peer, message);
    }
    on::EventButtonDataSet(event, true);
    on::EventButtonDataSet(event, false);
    send_varlist(event.peer, { "OnActiveAuctionUpdate", "{\"Active\":false}" });
    on::WorldLockStorageResponse(event.peer);
}
