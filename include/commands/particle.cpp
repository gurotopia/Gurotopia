#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "particle.hpp"

/* /particle {effect}            -> fires effect id at your position (colour 0x0e)
 * /particle {effect} {colour}   -> same, with a specific colour byte
 * handy for finding the id you want; values are shown in decimal and hex.
 */
void particle(ENetEvent& event, const std::string_view text)
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

    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /particle {effect} [colour]``");
        return;
    }

    std::string effect_str{}, colour_str{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        effect_str = arg.substr(0, sp);
        colour_str = arg.substr(sp + 1);
    }
    else effect_str = arg;

    auto parse = [](std::string s, signed fallback) -> signed
    {
        std::erase_if(s, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
        if (s.empty()) return fallback;
        try { return std::stoi(s); } catch (...) { return fallback; }
    };

    const signed effect = parse(effect_str, 0);
    const signed colour = parse(colour_str, 0x0e);

    send_particle_effect(event, pPeer->pos, { colour, effect });

    on::ConsoleMessage(event.peer,
        std::format("`2Effect `w{}`` (0x{:02x}) with colour `w{}`` (0x{:02x}).``",
                    effect, effect, colour, colour));
}
