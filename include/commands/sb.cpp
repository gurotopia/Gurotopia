#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "jammers.hpp"
#include "sb.hpp"

void sb(ENetEvent& event, const std::string_view text)
{
    if (text.size() <= sizeof("sb ") - 1) return;
    const std::string message{ text.substr(sizeof("sb ") - 1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    // @note mods and devs always show as JAMMED, and so does a world with a jammer
    const bool jammed = pPeer->role >= MODERATOR || !jam_suffix(world->name).empty();
    const std::string where = jammed ? "`4JAMMED!" : world->name;
    const char *chat_color = (pPeer->role >= DEVELOPER) ? "`5" : (pPeer->role >= MODERATOR) ? "`^" : "`w"; // @note same colour as normal chat
    const std::string name = // @note rank colour, not the world colour (owner green etc.)
        !pPeer->nickname.empty()        ? std::format("`w{}``", pPeer->nickname) :
        pPeer->role >= DEVELOPER        ? std::format("`6@{}``", pPeer->growid) :
        pPeer->role >= MODERATOR        ? std::format("`5@{}``", pPeer->growid) :
                                          std::format("`w{}``", pPeer->growid);
    const std::string line = std::format(
        "CP:0_PL:0_OID:_CT:[SB]_ `5** from ({}```5) in [```${}```5] ** : ``{}{}``",
        name, where, chat_color, message);

    peers("", PEER_ALL, [&line](ENetPeer &peer) { on::ConsoleMessage(&peer, line); }); // @note to everyone online, not just you
}