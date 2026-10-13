#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "action/trade.hpp"
#include "trade.hpp"
#include <cctype>

namespace
{
    ENetPeer *find_in_world(const std::string &world, const std::string &name, bool &ambiguous)
    {
        auto lower = [](std::string s) { for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; };
        ambiguous = false;
        const std::string want = lower(name);
        ENetPeer *exact = nullptr, *prefix = nullptr;
        int prefixes = 0;
        peers(world, PEER_SAME_WORLD, [&](ENetPeer &p)
        {
            const ::peer *t = static_cast<::peer*>(p.data);
            if (!t || t->growid.empty()) return;
            const std::string have = lower(t->growid);
            if (have == want) exact = &p;
            else if (want.size() >= 3 && have.starts_with(want)) { prefix = &p; ++prefixes; }
        });
        if (exact) return exact;
        if (prefixes > 1) ambiguous = true;
        return prefixes == 1 ? prefix : nullptr;
    }

}

void trade_cmd(ENetEvent &event, const std::string_view text)
{
    if (!event.peer || !event.peer->data) return;
    const ::peer &player = *static_cast<::peer*>(event.peer->data);
    if (player.user_id <= 0 || player.netid == 0) return;
    std::string name{text};
    const auto space = name.find(' ');
    name = space == std::string::npos ? "" : name.substr(space + 1);
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
    if (name.empty())
    {
        send_action(*event.peer, "log", "msg|To trade with a specific person in this world, do `2/trade <``full or partial name`2>``");
        return;
    }
    bool ambiguous{};
    ENetPeer *target = find_in_world(player.recent_worlds.back(), name, ambiguous);
    if (!target)
    {
        on::ConsoleMessage(event.peer, ambiguous ? "`4Oops:`` More than one player starts with that - type more letters." :
            std::format("`4Oops:`` There is nobody currently in this world with a name starting with `w{}``.", name));
        return;
    }
    on::ConsoleMessage(event.peer, std::format("Trading with {}...", static_cast<::peer*>(target->data)->display_growid));
    trade_request(event, *target);
}
