#include "pch.hpp"

#include "onVariant/WorldLockStorageResponse.hpp"

#include "worldlock_storage_getpage.hpp"

void action::worldlock_storage_getpage(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    on::WorldLockStorageResponse(event.peer);
}
