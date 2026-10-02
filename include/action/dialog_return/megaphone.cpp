#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/sb.hpp"
#include "megaphone.hpp"

void megaphone(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->curse_until > std::time(nullptr))
    {
        tell(event.peer, "`4You can't do that while cursed.``");
        return;
    }

    std::string message = hPipe["message"];
    if (message.empty()) return;
    sb(event, "sb " + message); // @note /sb skips the first 3 chars ("sb ")
}
