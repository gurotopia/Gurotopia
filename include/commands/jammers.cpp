#include "pch.hpp"
#include <chrono>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "tools/bubble.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "jammers.hpp"

static constexpr u_short SIGNAL = 226, PUNCH = 1276, ZOMBIE = 1278, ANTIGRAVITY = 4992;

bool is_jammer(u_short id) { return id == SIGNAL || id == PUNCH || id == ZOMBIE || id == ANTIGRAVITY; }

/* is there a switched-on jammer of this kind in the world? */
bool world_jammer(const ::world &w, u_short id)
{
    for (const ::block &b : w.blocks) if (b.fg == id && (b.state[2] & S_TOGGLE)) return true;
    return false;
}

bool world_jammer(const std::string &world_name, u_short id)
{
    auto w = std::ranges::find(worlds, world_name, &::world::name);
    return w != worlds.end() && world_jammer(*w, id);
}

/* the owner punches a jammer: switch it. @return always false, so the punch still counts as damage */
bool jammer_punch(ENetEvent &event, ::world &w, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const int x = static_cast<int>(gamePacket.punch.x), y = static_cast<int>(gamePacket.punch.y);
    if (x < 0 || y < 0 || x >= static_cast<int>(w.blocks.size() / 60) || y >= 60) return false;
    ::block &b = w.blocks[cord(x, y)];
    if (!is_jammer(b.fg)) return false;
    if (w.owner == 0)
    {
        tell(event.peer, "`4Jammers only work in a world with a World Lock.``");
        return false;
    }
    if (w.owner != pPeer->user_id && pPeer->role < DEVELOPER && !(b.state[2] & S_PUBLIC)) return false; // @note public: anyone may switch it

    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_switch{};
    const std::string key = std::format("{}:{}:{}", w.name, x, y);
    const auto now = std::chrono::steady_clock::now();
    if (auto it = last_switch.find(key); it != last_switch.end() && now - it->second < std::chrono::milliseconds(250))
        return false; // @note switched moments ago: a duplicated punch
    last_switch[key] = now;

    b.state[2] ^= S_TOGGLE;
    const bool on = (b.state[2] & S_TOGGLE) != 0;
    // @note no tile update here: the game flips the light itself on the punch

    const std::string msg =
        (b.fg == SIGNAL) ? (on ? "`2Signal Jammer on:`` this world is hidden from the world list." : "`4Signal Jammer off:`` this world shows in the world list again.") :
        (b.fg == PUNCH)  ? (on ? "`2Punch Jammer on:`` players can't be hit with items here." : "`4Punch Jammer off:`` players can be hit again.") :
        (b.fg == ZOMBIE) ? (on ? "`2Zombie Jammer on:`` zombies can't spread here." : "`4Zombie Jammer off:`` zombies can spread again.") :
                           (on ? "`2Antigravity Generator on:`` low gravity - jump as much as you like!" : "`4Antigravity Generator off:`` gravity is back to normal.");
    const bool gravity = (b.fg == ANTIGRAVITY);
    peers(w.name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        on::ConsoleMessage(&p, msg);
        if (gravity && static_cast<::peer*>(p.data)->netid != 0) on::SetClothing(p);
    });
    return false;
}

/* items stopped by a Punch or Zombie Jammer. @return true if blocked */
bool jammer_blocks(ENetEvent &event, const ::world &w, short item_id)
{
    static constexpr short harmful[]{ 274, 276, 338, 368, 614, 618, 764, 874, 962, 1368, 1988, 4754 };
    if (item_id == 764 && world_jammer(w, ZOMBIE))
    {
        tell(event.peer, "`4A Zombie Jammer stops that in this world.``");
        return true;
    }
    if (std::ranges::find(harmful, item_id) != std::end(harmful) && world_jammer(w, PUNCH))
    {
        tell(event.peer, "`4A Punch Jammer protects players in this world.``");
        return true;
    }
    return false;
}


/* " [JAMMED!]" (red) if a Signal, Punch or Zombie Jammer is on, else nothing */
std::string jam_suffix(const std::string &world_name)
{
    auto w = std::ranges::find(worlds, world_name, &::world::name);
    if (w == worlds.end()) return {};
    return (world_jammer(*w, 226) || world_jammer(*w, 1276) || world_jammer(*w, 1278)) ? " [`4JAMMED!``]" : "";
}

/* wrench: owner gets Public + Silence, others just the status */
void jammer_wrench(ENetEvent &event, ::world &w, ::block &b, ::gamePacket &gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    const ::item &it = id_to_item(b.fg);
    const bool on = (b.state[2] & S_TOGGLE) != 0;
    const bool owner = (w.owner != 0 && w.owner == pPeer->user_id) || pPeer->role >= DEVELOPER;

    std::string d = std::format(
        "set_default_color|`o\n"
        "add_label_with_icon|big|`w{}``|left|{}|\n"
        "add_textbox|It is currently {}.|left|\n",
        it.raw_name, it.id, on ? "`2ON``" : "`4OFF``");
    if (owner)
    {
        d += std::format(
            "add_checkbox|checkbox_public|Usable by public|{}|\n"
            "add_checkbox|checkbox_silence|Silence|{}|\n"
            "embed_data|tilex|{}\n"
            "embed_data|tiley|{}\n"
            "end_dialog|jammer_edit|Cancel|OK|\n",
            (b.state[2] & S_PUBLIC) ? 1 : 0, (b.state[3] & 0x02) ? 1 : 0,
            static_cast<int>(gamePacket.punch.x), static_cast<int>(gamePacket.punch.y));
    }
    else d += "end_dialog|jammer_edit|Close||\n";
    send_varlist(event.peer, { "OnDialogRequest", d });
}

void jammer_edit_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;
    if (!((w->owner != 0 && w->owner == pPeer->user_id) || pPeer->role >= DEVELOPER)) return;

    const std::string sx = hPipe["tilex"], sy = hPipe["tiley"];
    if (sx.empty() || sy.empty()) return;
    const int x = std::atoi(sx.c_str()), y = std::atoi(sy.c_str());
    if (x < 0 || y < 0 || x >= static_cast<int>(w->blocks.size() / 60) || y >= 60) return;
    ::block &b = w->blocks[cord(x, y)];
    if (!is_jammer(b.fg)) return;

    if (hPipe["checkbox_public"] == "1") b.state[2] |= S_PUBLIC; else b.state[2] &= ~S_PUBLIC;
    if (hPipe["checkbox_silence"] == "1") b.state[3] |= 0x02; else b.state[3] &= ~0x02; // @note "alternate mode" flag = silenced
    send_tile_update(event, { .id = b.fg, .punch = ::pos{ x, y } }, b, *w);
    tell(event.peer, "`2Settings saved.``");
}