#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/trade.hpp"

#include "trade.hpp"

void trade(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string name{ text.substr(sizeof("trade ")-1) };
    for (char &c : name) c = std::tolower(c); // @note growid isn't case sensitive

    ENetPeer *exact = nullptr;
    std::vector<ENetPeer*> starts_with{};
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&name, &exact, &starts_with](ENetPeer& peer)
    {
        std::string growid = static_cast<::peer*>(peer.data)->growid;
        for (char &c : growid) c = std::tolower(c);

        if (growid == name) exact = &peer;
        else if (growid.starts_with(name)) starts_with.emplace_back(&peer);
    });
    if (exact == nullptr && starts_with.size() == 1) exact = starts_with.front(); // @note "/trade lee" finds leeendl

    if (exact != nullptr) trade_request(event, *exact);
    else if (starts_with.size() > 1) on::ConsoleMessage(event.peer, "`4Oops:`` More than one player starts with that - type more letters.");
    else on::ConsoleMessage(event.peer, std::format("`4Oops:`` There is nobody currently in this world with a name starting with `w{}``.", name));
}
