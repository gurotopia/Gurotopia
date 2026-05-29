#include "pch.hpp"

#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "on/NameChanged.hpp"
#include "database/peer.hpp"

#include "birth_certificate.hpp"
#include <unordered_set>

namespace
{
    constexpr long long rename_cooldown_seconds = 15ll * 24ll * 60ll * 60ll;

    std::string lower_copy(std::string value)
    {
        std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    std::string escaped_for_dialog(std::string value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char c : value)
        {
            switch (c)
            {
                case '`': escaped += "``"; break;
                case '|': escaped += "/"; break;
                case '\n':
                case '\r': break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }

    bool name_exists_case_insensitive(const std::string& name)
    {
        sqlite3* db = nullptr;
        if (sqlite3_open("db/peers.db", &db) != SQLITE_OK) return false;

        bool found = false;
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, "SELECT 1 FROM peers WHERE LOWER(_n) = LOWER(?) LIMIT 1", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            found = sqlite3_step(stmt) == SQLITE_ROW;
            sqlite3_finalize(stmt);
        }

        if (!found && sqlite3_prepare_v2(db, "SELECT 1 FROM peer_aliases WHERE LOWER(alias) = LOWER(?) LIMIT 1", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            found = sqlite3_step(stmt) == SQLITE_ROW;
            sqlite3_finalize(stmt);
        }

        sqlite3_close(db);
        return found;
    }

    short birth_certificate_item_id()
    {
        auto it = std::ranges::find_if(items, [](const ::item& item)
        {
            return item.raw_name == "Birth Certificate";
        });
        return (it != items.end()) ? static_cast<short>(it->id) : 0;
    }

    bool has_birth_certificate(::peer& peer)
    {
        const short id = birth_certificate_item_id();
        if (id == 0) return false;

        auto it = std::ranges::find(peer.slots, id, &::slot::id);
        return it != peer.slots.end() && it->count > 0;
    }

    std::string remaining_text(long long seconds)
    {
        const long long days = seconds / 86400;
        const long long hours = (seconds % 86400) / 3600;
        if (days > 0) return std::format("{} day(s) and {} hour(s)", days, hours);
        const long long minutes = std::max(1ll, (seconds % 3600) / 60);
        return std::format("{} hour(s) and {} minute(s)", hours, minutes);
    }

    void show_birth_certificate_dialog(ENetEvent& event, const std::string& attempted_name = {}, const std::string& error = {})
    {
        std::string dialog;
        dialog += ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wBirth Certificate``", birth_certificate_item_id())
            .add_spacer("small")
            .end_dialog("__noop__", "", "");

        const std::string noop = "end_dialog|__noop__|||\n";
        if (dialog.size() >= noop.size() && dialog.ends_with(noop))
            dialog.erase(dialog.size() - noop.size());

        if (!error.empty())
        {
            dialog += std::format("add_textbox|`4{}|left|\n", escaped_for_dialog(error));
            dialog += "add_spacer|small|\n";
        }

        dialog += ::create_dialog()
            .add_textbox("Choose your new GrowID. This can only be changed once every 15 days.")
            .add_spacer("small")
            .add_text_input("new_name", "New GrowID:", attempted_name, 20)
            .add_spacer("small")
            .end_dialog("birth_certificate", "Cancel", "Use");

        packet::create(*event.peer, false, 0, { "OnDialogRequest", dialog.c_str() });
    }

    bool fail_birth_certificate(ENetEvent& event, const std::string& attempted_name, const std::string& error)
    {
        show_birth_certificate_dialog(event, attempted_name, error);
        return false;
    }
}

void birth_certificate(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer* peer = static_cast<::peer*>(event.peer->data);
    if (!peer) return;

    std::string new_name;
    if (pipes.size() > 5zu) new_name = pipes[5zu];

    new_name.erase(std::remove_if(new_name.begin(), new_name.end(), [](unsigned char c)
    {
        return std::isspace(c);
    }), new_name.end());

    if (!has_birth_certificate(*peer))
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "`4You need a Birth Certificate in your inventory!``" });
        return;
    }

    const long long now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const long long next_rename_at = peer->name_changed_at + rename_cooldown_seconds;
    if (peer->name_changed_at != 0 && now < next_rename_at)
    {
        fail_birth_certificate(event, new_name, std::format("You can change your name again in {}.", remaining_text(next_rename_at - now)));
        return;
    }

    if (new_name.length() < 3 || new_name.length() > 20)
    {
        fail_birth_certificate(event, new_name, "Your new name must be between 3 and 20 characters.");
        return;
    }

    if (!alnum(new_name))
    {
        fail_birth_certificate(event, new_name, "Your new name can only contain letters and numbers.");
        return;
    }

    static const std::unordered_set<std::string> reserved_names{
        "mod", "moderator", "admin", "administrator", "dev", "developer", "owner", "system", "console", "server"
    };
    if (reserved_names.contains(lower_copy(new_name)))
    {
        fail_birth_certificate(event, new_name, "That name is reserved.");
        return;
    }

    if (lower_copy(peer->ltoken[0]) == lower_copy(new_name))
    {
        fail_birth_certificate(event, new_name, "That is already your current name.");
        return;
    }

    for (ENetPeer* connected : peers())
    {
        ::peer* online = static_cast<::peer*>(connected->data);
        if (online && lower_copy(online->ltoken[0]) == lower_copy(new_name))
        {
            fail_birth_certificate(event, new_name, std::format("\"{}\" already is a cool name, but is already in use!", new_name));
            return;
        }
    }

    if (name_exists_case_insensitive(new_name))
    {
        fail_birth_certificate(event, new_name, std::format("\"{}\" already is a cool name, but is already in use!", new_name));
        return;
    }

    const std::string old_name = peer->ltoken[0];
    if (!peer->rename(old_name, new_name))
    {
        fail_birth_certificate(event, new_name, "Failed to rename your account.");
        return;
    }

    peer->ltoken[0] = new_name;
    peer->name_changed_at = now;

    const short certificate_id = birth_certificate_item_id();
    if (certificate_id != 0) modify_item_inventory(event, ::slot(certificate_id, -1));

    on::NameChanged(event);
    packet::create(*event.peer, false, 0, { "SetHasGrowID", 1, peer->ltoken[0].c_str(), "" });
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("`2Your name was changed from `w{}`` to `w{}``! You can change it again in 15 days.", old_name, peer->ltoken[0]).c_str()
    });

    if (!peer->recent_worlds.back().empty())
    {
        const std::string msg = std::format("`5<`w{} ``is now known as `w{}``>``", old_name, peer->ltoken[0]);
        peers(peer->recent_worlds.back(), PEER_SAME_WORLD, [&msg](ENetPeer& p)
        {
            packet::create(p, false, 0, { "OnConsoleMessage", msg.c_str() });
        });
    }
}
