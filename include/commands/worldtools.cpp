#include "pch.hpp"
#include "tools/bubble.hpp"
#include <fstream>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/NameChanged.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tools/create_dialog.hpp"
#include "tools/string.hpp"
#include "database/database.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "dropqueue.hpp"
#include "worldtools_use.hpp"

static constexpr short BIRTH_CERTIFICATE = 1280;
static constexpr short CHANGE_OF_ADDRESS = 2580;
static constexpr short LOCK_MOVER        = 3560;
static constexpr short DOOR_MOVER        = 1404;
static constexpr short GROWPEDIA         = 6336;
static constexpr short FLASHBACK_FLAN    = 6916;

static int count_of(const ::peer &p, short id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}

static std::string trimmed(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(0, 1);
    return s;
}

static std::string lower(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/* everyone in the world re-enters it, so they see changes to doors/locks */
static void reload_world_for_everyone(const std::string &name)
{
    peers(name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ENetEvent fake{ .peer = &p };
        action::quit_to_exit(fake, "", true);
        action::join_request(fake, "", name);
    });
}

bool tool_use(ENetEvent &event, ::world &world, const ::item &item, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const short id = static_cast<short>(item.id);
    const int x = static_cast<int>(gamePacket.punch.x);
    const int y = static_cast<int>(gamePacket.punch.y);
    const int width = static_cast<int>(world.blocks.size() / 60);

    switch (id)
    {
        case GROWPEDIA:
        {
            send_varlist(event.peer, { "OnDialogRequest",
                ::create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", "`wGrowpedia``", GROWPEDIA)
                    .add_spacer("small")
                    .add_textbox("Welcome to Growtopia! A few tips to get you started:")
                    .add_smalltext("- Punch blocks to break them and collect seeds and gems.")
                    .add_smalltext("- Plant a seed on top of another seed to splice them into something new.")
                    .add_smalltext("- Get a `wWorld Lock`` to protect your world.")
                    .add_smalltext("- Type `w/?`` to see the commands you can use.")
                    .end_dialog("growpedia", "Close", "") });
            return true;
        }
        case FLASHBACK_FLAN:
        {
            if (x < 0 || y < 0 || x >= width || y >= 60) return true;
            if (world.owner != pPeer->user_id)
            {
                tell(event.peer, "`4You can only use that in a world you own.``");
                return true;
            }
            ::block &b = world.blocks[cord(x, y)];
            const bool fg = b.fg != 0;
            const short target = fg ? b.fg : b.bg;
            if (target == 0)
            {
                tell(event.peer, "`4Use that on a block.``");
                return true;
            }
            const ::item &t = id_to_item(target);
            if (target == 8 || t.type == type::MAIN_DOOR || t.type == type::LOCK || t.type == type::SEED || t.type == type::STRONG || get_type(t) != 0)
            {
                tell(event.peer, "`4That block can't be rewound.``");
                return true;
            }
            modify_item_inventory(event, ::slot(id, -1));
            modify_item_inventory(event, ::slot(target, 1));
            if (fg) b.fg = 0; else b.bg = 0;
            send_tile_update(event, ::gamePacket{ .punch = ::pos{ x, y } }, b, world);
            on::ConsoleMessage(event.peer, std::format("`2Time rewinds... the `w{}`` is back in your backpack!``", t.raw_name));
            return true;
        }
        case DOOR_MOVER:
        {
            if (x < 0 || y < 0 || x >= width || y + 1 >= 60) return true;
            if (world.owner != pPeer->user_id)
            {
                tell(event.peer, "`4You can only use that in a world you own.``");
                return true;
            }
            if (world.blocks[cord(x, y)].fg != 0 || world.blocks[cord(x, y + 1)].fg != 0)
            {
                tell(event.peer, "`4There's no room to put the door there! You need 2 empty spaces vertically.``");
                return true;
            }

            ::pos old_pos{ -1, -1 };
            for (std::size_t i = 0; i < world.blocks.size(); ++i)
                if (world.blocks[i].fg == 6) // @note Main Door
                {
                    const int ox = static_cast<int>(i % width), oy = static_cast<int>(i / width);
                    old_pos = ::pos{ ox, oy };
                    world.blocks[i].fg = 0;
                    if (oy + 1 < 60 && world.blocks[cord(ox, oy + 1)].fg == 8) world.blocks[cord(ox, oy + 1)].fg = 0;
                    break;
                }
            const ::pos new_pos{ x, y };
            std::erase_if(world.doors, [&](const ::door &d) { return d.pos == new_pos; }); // @note clear any leftover record
            auto door = std::ranges::find(world.doors, old_pos, &::door::pos);
            if (door != world.doors.end()) door->pos = new_pos;
            else world.doors.emplace_back(::door("EXIT", "", "", new_pos));

            world.blocks[cord(x, y)].fg = 6;
            world.blocks[cord(x, y + 1)].fg = 8;
            world.spawn = new_pos.by_32(false);
            on::ConsoleMessage(event.peer, std::format("`oDoor moved from (`w{},{}``) to (`w{},{}``).``", static_cast<int>(old_pos.x), static_cast<int>(old_pos.y), x, y));

            modify_item_inventory(event, ::slot(id, -1));
            const std::string name = world.name;
            reload_world_for_everyone(name);
            return true;
        }        case LOCK_MOVER:
        {
            if (x < 0 || y < 0 || x >= width || y >= 60) return true;
            if (world.owner != pPeer->user_id)
            {
                tell(event.peer, "`4You can only use that in a world you own.``");
                return true;
            }
            ::block &dest = world.blocks[cord(x, y)];
            if (dest.fg != 0)
            {
                tell(event.peer, "`4There's no room to put the lock there!``");
                return true;
            }
            ::block *lock = nullptr;
            for (::block &b : world.blocks)
                if (id_to_item(b.fg).type == type::LOCK && !is_tile_lock(b.fg)) { lock = &b; break; }
            if (!lock)
            {
                tell(event.peer, "`4This world has no World Lock to move.``");
                return true;
            }
            dest.fg = lock->fg;
            lock->fg = 0;
            modify_item_inventory(event, ::slot(id, -1));
            const std::string name = world.name;
            reload_world_for_everyone(name);
            return true;
        }
        case BIRTH_CERTIFICATE:
        {
            const std::time_t now = std::time(nullptr);
            const long long wait = 60LL * 86400 - (static_cast<long long>(now) - static_cast<long long>(pPeer->renamed_at));
            if (pPeer->renamed_at != 0 && wait > 0)
            {
                tell(event.peer, std::format("`4You can only change your name once every 60 days.`` Try again in `w{}`` days.", (wait + 86399) / 86400));
                return true;
            }
            send_varlist(event.peer, { "OnDialogRequest",
                ::create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", "`wBirth Certificate``", BIRTH_CERTIFICATE)
                    .add_textbox("Choose your new GrowID. It must be 3-18 letters and numbers, and not already taken. Your name changes right away.")
                    .add_text_input("name", "New GrowID:", std::string{}, 18)
                    .end_dialog("birth_certificate", "Cancel", "Change Name") });
            return true;
        }
        case CHANGE_OF_ADDRESS:
        {
            if (world.owner != pPeer->user_id)
            {
                tell(event.peer, "`4You must own this world to swap its name.``");
                return true;
            }
            send_varlist(event.peer, { "OnDialogRequest",
                ::create_dialog()
                    .set_default_color("`o")
                    .add_label_with_icon("big", "`wChange of Address``", CHANGE_OF_ADDRESS)
                    .add_textbox(std::format("Enter the name of another world you own. It will swap names with `w{}``. You must be alone here, and nobody can be in the other world.", world.name))
                    .add_text_input("world", "Other world:", std::string{}, 24)
                    .end_dialog("change_address", "Cancel", "Swap Names") });
            return true;
        }
    }
    return false;
}

void birth_certificate_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (count_of(*pPeer, BIRTH_CERTIFICATE) < 1) return;

    const std::string name = trimmed(hPipe["name"]);
    if (name.size() < 3 || name.size() > 18 || !alnum(name))
    {
        tell(event.peer, "`4Names must be 3-18 letters and numbers only.``");
        return;
    }
    if (lower(name) == lower(pPeer->growid))
    {
        tell(event.peer, "`4That's already your name!``");
        return;
    }
    if (pPeer->exists(name))
    {
        tell(event.peer, "`4That name is already taken.``");
        return;
    }

    const std::string old = pPeer->growid;
    {
        ::hStmt h{ "UPDATE peer SET growid = ? WHERE growid = ?" };
        MYSQL_BIND params[2] = { make_bind_in(name), make_bind_in(old) };
        h.bind_param(params);
        h.execute();
    }
    {
        ::hStmt h{ "UPDATE renames SET new_name = ? WHERE new_name = ?" }; // @note older names now point to the newest
        MYSQL_BIND params[2] = { make_bind_in(name), make_bind_in(old) };
        h.bind_param(params);
        h.execute();
    }
    {
        ::hStmt h{ "DELETE FROM renames WHERE old_name = ?" }; // @note the new name is a real account now
        MYSQL_BIND param = make_bind_in(name);
        h.bind_param(&param);
        h.execute();
    }
    {
        ::hStmt h{ "REPLACE INTO renames (old_name, new_name) VALUES (?, ?)" };
        MYSQL_BIND params[2] = { make_bind_in(old), make_bind_in(name) };
        h.bind_param(params);
        h.execute();
    }    pPeer->growid = name;
    const std::time_t now = std::time(nullptr);
    pPeer->renamed_at = now;
    pPeer->mysql_update("renamed_at", static_cast<signed>(now));
    modify_item_inventory(event, ::slot(BIRTH_CERTIFICATE, -1));

    send_varlist(event.peer, { "SetHasGrowID", 1, name.c_str(), pPeer->password.c_str() });
    on::ConsoleMessage(event.peer, std::format("`2Your GrowID is now `w{}``!``", name));
    printf("[rename] %s -> %s\n", old.c_str(), name.c_str());
    if (pPeer->nickname.empty())
    {
        auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        pPeer->display_growid =
            (pPeer->role >= DEVELOPER) ? std::format("`6@{}``", name) :
            (pPeer->role >= MODERATOR) ? std::format("`5@{}``", name) :
            (w != worlds.end() && w->owner == pPeer->user_id) ? std::format("`2{}``", name) :
                                                                 std::format("`w{}``", name);
    }
    if (pPeer->netid != 0) on::NameChanged(event);
}

void change_address_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->netid == 0 || count_of(*pPeer, CHANGE_OF_ADDRESS) < 1) return;

    std::string other = trimmed(hPipe["world"]);
    for (char &c : other) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (other.empty() || other.size() > 24 || !alnum(other))
    {
        tell(event.peer, "`4That's not a valid world name.``");
        return;
    }
    const std::string current = pPeer->recent_worlds.back();
    if (other == current)
    {
        tell(event.peer, "`4Pick a different world.``");
        return;
    }
    auto cur = std::ranges::find(worlds, current, &::world::name);
    if (cur == worlds.end()) return;
    if (cur->owner != pPeer->user_id)
    {
        tell(event.peer, "`4You must own this world.``");
        return;
    }
    if (cur->visitors > 1)
    {
        tell(event.peer, "`4Everyone else has to leave this world first.``");
        return;
    }
    if (std::ranges::find(worlds, other, &::world::name) != worlds.end())
    {
        tell(event.peer, std::format("`4Nobody can be in `w{}`` while you swap names.``", other));
        return;
    }
    if (!cur->exists(other))
    {
        tell(event.peer, std::format("`4The world `w{}`` doesn't exist.``", other));
        return;
    }
    int other_owner = 0;
    {
        ::world probe(other); // @note loads it just to read the owner, saved again when it goes out of scope
        other_owner = probe.owner;
    }
    if (other_owner != pPeer->user_id)
    {
        tell(event.peer, std::format("`4You must own `w{}`` too.``", other));
        return;
    }

    modify_item_inventory(event, ::slot(CHANGE_OF_ADDRESS, -1));
    action::quit_to_exit(event, "", true); // @note unloads and saves this world

    auto rename = [](const std::string &from, const std::string &to)
    {
        ::hStmt h{ "UPDATE world SET name = ? WHERE name = ?" };
        MYSQL_BIND params[2] = { make_bind_in(to), make_bind_in(from) };
        h.bind_param(params);
        h.execute();
    };
    const std::string tmp = "~SWAP~";
    rename(current, tmp);
    rename(other, current);
    rename(tmp, other);
    printf("[address] swapped %s <-> %s\n", current.c_str(), other.c_str());

    action::join_request(event, "", current);
    on::ConsoleMessage(event.peer, std::format("`2Done!`` `w{}`` and `w{}`` have swapped names.", current, other));
}


/* /tile [x y] -> what the server has stored at a tile (developers) */
void tile_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER || pPeer->netid == 0) return;

    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;
    const int width = static_cast<int>(w->blocks.size() / 60);

    int x = static_cast<int>((pPeer->pos.x + 10.0f) / 32.0f);
    int y = static_cast<int>((pPeer->pos.y + 15.0f) / 32.0f) + 1; // @note the tile under your feet
    std::string args{ text };
    if (const std::size_t sp = args.find(' '); sp != std::string::npos)
    {
        args = args.substr(sp + 1);
        const std::size_t sp2 = args.find(' ');
        if (sp2 != std::string::npos) { x = std::atoi(args.substr(0, sp2).c_str()); y = std::atoi(args.substr(sp2 + 1).c_str()); }
    }
    if (x < 0 || y < 0 || x >= width || y >= 60)
    {
        tell(event.peer, "`4Outside the world.``");
        return;
    }
    const ::block &b = w->blocks[cord(x, y)];
    on::ConsoleMessage(event.peer, std::format("`oTile (`w{},{}``): block `w{}`` ({}), background `w{}`` ({})``",
        x, y, b.fg, id_to_item(b.fg).raw_name, b.bg, id_to_item(b.bg).raw_name));
}

/* /dumpitems -> writes every non-seed item to items_list.txt (developers) */
void dumpitems_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;

    std::ofstream out("items_list.txt");
    int n = 0;
    for (const ::item &it : items)
    {
        if (it.type == type::SEED || it.raw_name.empty()) continue;
        out << it.id << '|' << it.raw_name << '|' << static_cast<int>(it.type) << '\n';
        ++n;
    }
    on::ConsoleMessage(event.peer, std::format("`2Wrote `w{}`` items to items_list.txt``", n));
}

/* worlds that /deleteallworlds never touches */
static std::vector<std::string> worlds_to_keep(const std::vector<std::string> &extra)
{
    std::vector<std::string> keep{ "HELL", "START" };
    for (const ::world &w : worlds) keep.push_back(w.name);
    for (const std::string &e : extra) keep.push_back(e);
    return keep;
}

static std::string marks_for(std::size_t n)
{
    std::string marks{};
    for (std::size_t i = 0; i < n; ++i) marks += (i ? ", ?" : "?");
    return marks;
}

static int count_deletable(const std::vector<std::string> &keep)
{
    std::vector<MYSQL_BIND> params{};
    for (const std::string &k : keep) params.push_back(make_bind_in(k));
    int count = 0;
    ::hStmt h{ std::format("SELECT COUNT(*) FROM world WHERE name NOT IN ({})", marks_for(keep.size())).c_str() };
    h.bind_param(params.data());
    u_long length = 0;
    MYSQL_BIND result = make_bind_out(count);
    result.length = &length;
    mysql_stmt_bind_result(h.pStmt, &result);
    h.execute();
    h.fetch();
    return count;
}

static unsigned long long delete_worlds(const std::vector<std::string> &keep)
{
    std::vector<MYSQL_BIND> params{};
    for (const std::string &k : keep) params.push_back(make_bind_in(k));
    ::hStmt h{ std::format("DELETE FROM world WHERE name NOT IN ({})", marks_for(keep.size())).c_str() };
    h.bind_param(params.data());
    h.execute();
    return mysql_stmt_affected_rows(h.pStmt);
}

static std::unordered_map<std::string, std::vector<std::string>> pending_keep{}; // @note growid -> extra names to keep

/* /deleteallworlds [NAMES TO KEEP...] -> asks for confirmation in a menu (developers) */
void deleteallworlds_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;

    std::vector<std::string> extra{};
    {
        std::string rest{ text };
        const std::size_t sp = rest.find(' ');
        rest = (sp == std::string::npos) ? std::string{} : rest.substr(sp + 1);
        std::string word{};
        for (char ch : rest + ' ')
        {
            if (std::isspace(static_cast<unsigned char>(ch))) { if (!word.empty()) { extra.push_back(word); word.clear(); } }
            else word += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
    }

    const std::vector<std::string> keep = worlds_to_keep(extra);
    const int count = count_deletable(keep);
    if (count == 0)
    {
        on::ConsoleMessage(event.peer, "`oThere are no worlds to delete.``");
        return;
    }
    pending_keep[pPeer->growid] = extra;

    std::string kept{};
    for (const std::string &k : keep) kept += (kept.empty() ? "" : ", ") + k;
    send_varlist(event.peer, { "OnDialogRequest",
        ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`4Delete All Worlds``", 1402)
            .add_spacer("small")
            .add_textbox(std::format("This will delete `w{}`` worlds forever. This can't be undone!", count))
            .add_smalltext(std::format("Kept: `w{}``", kept))
            .end_dialog("delete_all_worlds", "Cancel", "Delete Worlds") });
}

/* the "Delete Worlds" button */
void delete_all_worlds_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;

    auto it = pending_keep.find(pPeer->growid);
    if (it == pending_keep.end()) return;
    const std::vector<std::string> extra = it->second;
    pending_keep.erase(it);

    const unsigned long long deleted = delete_worlds(worlds_to_keep(extra));
    printf("[deleteallworlds] %s deleted %llu worlds\n", pPeer->growid.c_str(), deleted);
    on::ConsoleMessage(event.peer, std::format("`2Deleted `w{}`` worlds.``", deleted));
}

/* /fillworld -> menu to fill every empty tile with a dropped item (developers) */
void fillworld_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;
    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }
    send_varlist(event.peer, { "OnDialogRequest",
        ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wFill World With Drops``", 112)
            .add_textbox("Choose an item, and a dropped stack of it is placed on every empty tile in this world.")
            .add_text_input("item", "Item (ID or exact name):", std::string{}, 40)
            .add_text_input("amount", "Amount per drop (1-200):", std::string{ "1" }, 3)
            .add_text_input("spacing", "Every Nth tile (1 = every tile):", std::string{ "1" }, 2)
            .end_dialog("fill_world", "Cancel", "Fill World") });
}

/* the "Fill World" button */
void fill_world_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER || pPeer->netid == 0) return;

    const std::string what = trimmed(hPipe["item"]);
    short id = 0;
    if (!what.empty() && std::ranges::all_of(what, [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)) != 0; }))
        id = static_cast<short>(std::atoi(what.c_str()));
    else
        for (const ::item &it : items)
            if (!it.raw_name.empty() && lower(it.raw_name) == lower(what)) { id = static_cast<short>(it.id); break; }
    if (id <= 0 || id >= static_cast<int>(items.size()) || id_to_item(static_cast<u_short>(id)).raw_name.empty())
    {
        tell(event.peer, "`4No item found with that ID or name.``");
        return;
    }

    const int amount  = std::clamp(std::atoi(hPipe["amount"].c_str()), 1, 200);
    const int spacing = std::clamp(std::atoi(hPipe["spacing"].c_str()), 1, 20);

    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;

    const int width = static_cast<int>(w->blocks.size() / 60);
    std::vector<std::pair<int, int>> spots{};
    for (int y = 0; y < 60 && spots.size() < 6000; ++y)
        for (int x = 0; x < width && spots.size() < 6000; ++x)
            if (x % spacing == 0 && y % spacing == 0 && w->blocks[cord(x, y)].fg == 0) spots.emplace_back(x, y);
    const int placed = static_cast<int>(spots.size());

    for (const auto &[x, y] : spots) queue_add_drop(w->name, static_cast<u_short>(id), static_cast<u_short>(amount), ::pos{ x * 32.0f + 8.0f, y * 32.0f + 8.0f }); // @note sent in batches

    const std::string item_name = id_to_item(static_cast<u_short>(id)).raw_name;
    on::ConsoleMessage(event.peer, std::format("`2Dropped `w{}`` stacks of `w{}x {}``.``", placed, amount, item_name));
}

/* /cleardrops -> removes every dropped item in your world (developers) */
void cleardrops_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;
    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }
    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;

    const std::size_t removed = w->objects.size();
    dropqueue_cancel_adds(w->name); // @note stop a fill that's still on its way
    for (const ::object &o : w->objects) queue_remove_drop(w->name, static_cast<int>(o.uid), pPeer->growid); // @note removed in batches
    on::ConsoleMessage(event.peer, std::format("`2Removed `w{}`` dropped items.``", removed));
}