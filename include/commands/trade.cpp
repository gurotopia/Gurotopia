#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "action/trade.hpp"
#include "action/trade_internal.hpp"
#include "trade.hpp"
#include <cctype>

namespace
{
    std::string lower(std::string text)
    {
        for (char &c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    bool prefix_match(const std::string &name, const std::string &prefix)
    {
        return prefix.size() >= 3 && name.starts_with(prefix);
    }

    struct name_matches
    {
        ENetPeer *exact{};
        ENetPeer *prefix{};
        int prefixes{};
    };

    void match_player(ENetPeer &connection, const std::string &name, name_matches &matches)
    {
        const ::peer *player = static_cast<::peer*>(connection.data);
        if (!player || player->growid.empty()) return;
        const std::string have = lower(player->growid);
        if (have == name) { matches.exact = &connection; return; }
        if (prefix_match(have, name)) { matches.prefix = &connection; ++matches.prefixes; }
    }

    ENetPeer *find_in_world(const std::string &world, const std::string &name, bool &ambiguous)
    {
        const std::string want = lower(name);
        name_matches matches;
        peers(world, PEER_SAME_WORLD, [&](ENetPeer &connection) { match_player(connection, want, matches); });
        ambiguous = !matches.exact && matches.prefixes > 1;
        if (matches.exact) return matches.exact;
        return matches.prefixes == 1 ? matches.prefix : nullptr;
    }

    std::string argument(std::string_view text)
    {
        const auto space = text.find(' ');
        if (space == std::string_view::npos) return {};
        std::string name{text.substr(space + 1)};
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
        return name;
    }

    void missing_player(ENetPeer &connection, const std::string &name, bool ambiguous)
    {
        on::ConsoleMessage(&connection, ambiguous ? "`4Oops:`` More than one player starts with that - type more letters." :
            std::format("`4Oops:`` There is nobody currently in this world with a name starting with `w{}``.", name));
    }
}

void trade_cmd(ENetEvent &event, const std::string_view text)
{
    if (!trade::valid_event(event)) return;
    const ::peer &player = trade::of(*event.peer);
    const std::string name = argument(text);
    if (name.empty())
    {
        send_action(*event.peer, "log", "msg|To trade with a specific person in this world, do `2/trade <``full or partial name`2>``");
        return;
    }
    bool ambiguous{};
    ENetPeer *target = find_in_world(player.recent_worlds.back(), name, ambiguous);
    if (!target) { missing_player(*event.peer, name, ambiguous); return; }
    on::ConsoleMessage(event.peer, std::format("Trading with {}...", trade::of(*target).display_growid));
    trade_request(event, *target);
}
