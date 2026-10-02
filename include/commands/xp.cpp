#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
#include "xp.hpp"

/* /xp {amount} -> grants yourself XP (dev, for testing) */
void xp_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /xp {amount}``");
        return;
    }

    signed amount{};
    try { amount = std::stoi(arg); }
    catch (...) { on::ConsoleMessage(event.peer, "`4That is not a valid number.``"); return; }

    amount = std::clamp(amount, 1, 60000);

    pPeer->add_xp(event, static_cast<u_short>(amount));

    on::ConsoleMessage(event.peer,
        std::format("`2Gained `w{}`` XP. Now level `w{}`` with `w{}`` XP.``",
                    amount, pPeer->level.front(), pPeer->level.back()));
}

/* /level {n} -> jumps straight to a level (dev, for testing) */
void level_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
    if (arg.empty())
    {
        on::ConsoleMessage(event.peer,
            std::format("`oYou are level `w{}`` with `w{}`` XP. Usage: /level {{1-125}}",
                        pPeer->level.front(), pPeer->level.back()));
        return;
    }

    signed target{};
    try { target = std::stoi(arg); }
    catch (...) { on::ConsoleMessage(event.peer, "`4That is not a valid number.``"); return; }

    target = std::clamp(target, 1, 125);

    pPeer->level.front() = static_cast<u_short>(target);
    pPeer->level.back() = 0;

    send_varlist(event.peer, { "OnPlayerLeveledUp", pPeer->level.front() });

    on::ConsoleMessage(event.peer, std::format("`2You are now level `w{}``.``", target));
}
