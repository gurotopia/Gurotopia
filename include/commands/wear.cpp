#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "wear.hpp"

/* /wear {itemID} -> equips a clothing item straight onto your character */
void wear(ENetEvent& event, const std::string_view text)
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
        on::ConsoleMessage(event.peer, "`4Usage: /wear {itemID}``");
        return;
    }

    signed id{};
    try { id = std::stoi(arg); }
    catch (...) { on::ConsoleMessage(event.peer, "`4That is not a valid item id.``"); return; }

    const ::item &it = id_to_item(static_cast<short>(id));

    if (it.cloth_type == clothing::NONE)
    {
        on::ConsoleMessage(event.peer, "`4That item can't be worn.``");
        return;
    }

    pPeer->clothing[it.cloth_type] = static_cast<float>(id);
    pPeer->update_effects();
    on::SetClothing(*event.peer);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p) { ::peer *o = static_cast<::peer*>(p.data); if (o && o->user_id != pPeer->user_id) on::SetClothing(p, *pPeer); });

    on::ConsoleMessage(event.peer, std::format("`2Now wearing item `w{}``.``", id));
}
