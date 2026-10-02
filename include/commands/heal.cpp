#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "heal.hpp"

/* /heal -> restores your health */
void heal(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->pain_hp = 10;

    on::ConsoleMessage(event.peer, "`2Health restored.``");
}
