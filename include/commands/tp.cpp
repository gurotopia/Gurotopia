#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "tp.hpp"

/* /tp {x} {y} -> teleports you to those tile coordinates in this world */
void tp(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
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

    std::string x_str{}, y_str{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        x_str = arg.substr(0, sp);
        y_str = arg.substr(sp + 1);
    }
    else
    {
        on::ConsoleMessage(event.peer, "`4Usage: /tp {x} {y}``");
        return;
    }

    auto digits = [](std::string s){ std::erase_if(s, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); }); return s; };
    x_str = digits(x_str);
    y_str = digits(y_str);

    if (x_str.empty() || y_str.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /tp {x} {y}``");
        return;
    }

    signed x{}, y{};
    try { x = std::stoi(x_str); y = std::stoi(y_str); }
    catch (...) { on::ConsoleMessage(event.peer, "`4Those are not valid numbers.``"); return; }

    if (x < 0 || x >= 100 || y < 0 || y >= 60)
    {
        on::ConsoleMessage(event.peer, "`4That is outside the world.``");
        return;
    }

    pPeer->pos = ::pos{ x * 32.0f, y * 32.0f };

    send_varlist(event.peer, {
        "OnSetPos",
        CL_Vec2f{ pPeer->pos.x, pPeer->pos.y }
    }, pPeer->netid);

    on::ConsoleMessage(event.peer, std::format("`2Teleported to `w{}``, `w{}``.``", x, y));
}
