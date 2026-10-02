#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "tools/random.hpp"
#include "firework.hpp"

/* /firework          -> one burst at your position
 * /firework {count}  -> several bursts scattered around you (max 20)
 */
void firework(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    signed count = 1;
    if (!arg.empty())
    {
        std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
        if (!arg.empty())
        {
            try { count = std::stoi(arg); } catch (...) { count = 1; }
        }
    }
    count = std::clamp(count, 1, 20);

    for (signed i = 0; i < count; ++i)
    {
        const ::pos at = (count == 1)
            ? pPeer->pos
            : ::pos{ pPeer->pos.x + static_cast<float>(RandomRange(0, 320)) - 160.0f,
                     pPeer->pos.y + static_cast<float>(RandomRange(0, 200)) - 160.0f };
        fireworks(event, at);
    }

    on::ConsoleMessage(event.peer,
        std::format("`2Launched `w{}`` firework{}.``", count, count == 1 ? "" : "s"));
}
