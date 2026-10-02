#include "pch.hpp"
#include <chrono>
#include <map>
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "tools/bubble.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "pull.hpp"
#include "trade.hpp"

namespace
{
struct offer { short id; int count; };

struct tstate
{
    int partner{};           // @note user id we want to trade with (0 = none)
    bool open{};             // @note both sides agreed, window is live
    bool accepted{};
    bool confirmed{};
    std::vector<offer> items{};
    std::chrono::steady_clock::time_point last_change{};
};
std::map<int, tstate> states{}; // @note {user id, state}

std::string clean(std::string s)
{
    for (char &c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

::peer *me_of(ENetEvent &event) { return static_cast<::peer*>(event.peer->data); }

/* someone in the same world as `me` by user id */
ENetPeer *peer_by_uid(const ::peer &me, int uid)
{
    ENetPeer *hit = nullptr;
    if (me.netid == 0 || uid == 0) return nullptr;
    peers(me.recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (t && t->user_id == uid) hit = &p;
    });
    return hit;
}

int owned(const ::peer &p, short id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}

std::string slots_text(const tstate &s)
{
    std::string out{};
    for (const offer &o : s.items) out += std::format("add_slot|{}|{}\n", o.id, o.count);
    return out;
}

std::string name_of(const ::peer &p) { return p.display_growid.empty() ? p.growid : p.display_growid; }

/* the trade window of `to`: their own offer and the partner's offer */
void send_status(ENetPeer &to, const ::peer &owner, const tstate &s, bool mine, const std::string &flags)
{
    const std::string text = mine ? "`oSelect an item from the inventory.``" : std::format("`w{}``'s offer.``", name_of(owner));
    send_varlist(&to, { "OnTradeStatus", owner.netid, "", text, slots_text(s) + flags });
}

/* items changed: both windows refresh and both "accept" ticks reset */
void refresh(ENetPeer &a, ENetPeer &b, bool announce)
{
    ::peer *A = static_cast<::peer*>(a.data), *B = static_cast<::peer*>(b.data);
    tstate &sa = states[A->user_id], &sb = states[B->user_id];
    sa.accepted = sb.accepted = false;
    sa.confirmed = sb.confirmed = false;
    const std::string flags = "\nlocked|0\nreset_locks|1\naccepted|0\n";
    send_status(a, *A, sa, true, flags);
    send_status(a, *B, sb, false, flags);
    send_status(b, *B, sb, true, flags);
    send_status(b, *A, sa, false, flags);
    if (announce)
    {
        send_varlist(&a, { "OnTextOverlay", "The deal has changed" });
        send_varlist(&b, { "OnTextOverlay", "The deal has changed" });
        send_action(a, "play_sfx", "file|audio/tile_removed.wav\ndelayMS|0");
        send_action(b, "play_sfx", "file|audio/tile_removed.wav\ndelayMS|0");
    }
}

/* someone ticked or unticked "accept" */
void send_accepts(ENetPeer &a, ENetPeer &b)
{
    ::peer *A = static_cast<::peer*>(a.data), *B = static_cast<::peer*>(b.data);
    const tstate &sa = states[A->user_id], &sb = states[B->user_id];
    for (ENetPeer *to : { &a, &b })
    {
        send_status(*to, *A, sa, to == &a, std::format("\nlocked|1\naccepted|{}\n", sa.accepted ? 1 : 0));
        send_status(*to, *B, sb, to == &b, std::format("\nlocked|1\naccepted|{}\n", sb.accepted ? 1 : 0));
    }
}

void reset_state(int uid) { states.erase(uid); }

/* closes the window for `p`. text shown as overlay + console (empty = nothing) */
void close_for(ENetPeer *p, const std::string &text)
{
    if (!p || !p->data) return;
    send_varlist(p, { "OnForceTradeEnd" });
    if (!text.empty())
    {
        on::ConsoleMessage(p, text);
        send_varlist(p, { "OnTextOverlay", text });
    }
    reset_state(static_cast<::peer*>(p->data)->user_id);
}

bool tradable(short id, std::string &why)
{
    if (id <= 0 || id >= static_cast<int>(items.size())) { why = "That's not an item."; return false; }
    const ::item &it = id_to_item(static_cast<u_short>(id));
    if (it.type == type::FIST || it.type == type::WRENCH || id == 18 || id == 32) { why = "You'd be sorry if you lost that!"; return false; }
    const bool wand = (id == 274 || id == 276 || id == 278 || id == 732); // @note Freeze, Fire, Curse and Ban Wand can be traded
    if ((it.cat & CAT_UNTRADEABLE) && !wand) { why = "That item can't be traded."; return false; }
    return true;
}

/* true if giving `out` and getting `in` still fits the backpack; `which` gets the item that overflows */
bool fits(const ::peer &p, const std::vector<offer> &in, const std::vector<offer> &out, short &which)
{
    int used = 0;
    for (const ::slot &s : p.slots) if (s.count > 0) ++used;
    for (const offer &o : in)
    {
        int after = owned(p, o.id) + o.count;
        for (const offer &x : out) if (x.id == o.id) after -= x.count;
        if (after > 200) { which = o.id; return false; }
        if (owned(p, o.id) <= 0) ++used;
    }
    for (const offer &o : out) if (owned(p, o.id) - o.count <= 0) --used;
    which = 0;
    return used <= p.slot_size;
}

std::string confirm_rows(const std::vector<offer> &list)
{
    if (list.empty()) return "add_textbox|`4Nothing!``|left|\n";
    std::string s{};
    for (const offer &o : list)
        s += std::format("add_label_with_icon|small|(`w{}``) {}|left|{}|\n", o.count, clean(id_to_item(static_cast<u_short>(o.id)).raw_name), o.id);
    return s;
}

void confirm_dialog(ENetPeer &to, const tstate &mine, const tstate &theirs)
{
    std::string d = "set_default_color|`o\nadd_label_with_icon|big|`wTrade Confirmation``|left|1366|\nadd_spacer|small|\n";
    d += "add_textbox|`4You'll give:``|left|\nadd_spacer|small|\n" + confirm_rows(mine.items);
    d += "add_spacer|small|\nadd_textbox|`2You'll get:``|left|\nadd_spacer|small|\n" + confirm_rows(theirs.items);
    if (theirs.items.empty())
        d += "add_spacer|small|\nadd_textbox|`4SCAM WARNING: ``You are about to do a trade without receiving anything in return. Once you do the trade you cannot get the items back.|left|\n"
             "add_textbox|`4Do you really want to do this?``|left|\n";
    d += "add_spacer|small|\nadd_button|accept|Do The Trade!|noflags|0|0|\nadd_button|back|Cancel|noflags|0|0|\nend_dialog|trade_confirm|||\n";
    send_varlist(&to, { "OnDialogRequest", d });
}

void execute(ENetPeer &a, ENetPeer &b)
{
    ::peer *P[2] = { static_cast<::peer*>(a.data), static_cast<::peer*>(b.data) };
    ENetPeer *pp[2] = { &a, &b };
    const std::vector<offer> give[2] = { states[P[0]->user_id].items, states[P[1]->user_id].items };

    for (int s = 0; s < 2; ++s)
        for (const offer &o : give[s])
            if (owned(*P[s], o.id) < o.count)
            {
                close_for(pp[0], "`4The trade failed:`` an item isn't there anymore.");
                close_for(pp[1], "`4The trade failed:`` an item isn't there anymore.");
                return;
            }

    for (int s = 0; s < 2; ++s)
    {
        ENetEvent ev{};
        ev.peer = pp[s];
        bool clothes = false;
        for (const offer &o : give[s])
        {
            for (float c : P[s]->clothing) if (static_cast<short>(c) == o.id && owned(*P[s], o.id) - o.count <= 0) clothes = true;
            modify_item_inventory(ev, ::slot(o.id, static_cast<short>(-o.count)));
        }
        for (const offer &o : give[1 - s]) modify_item_inventory(ev, ::slot(o.id, static_cast<short>(o.count)));
        if (clothes) { P[s]->update_effects(); on::SetClothing(*pp[s]); } // @note gave away something they were wearing
    }

    // for show: every traded item flies from the giver to the receiver, for everyone in the world
    for (int s = 0; s < 2; ++s)
        for (const offer &o : give[s])
        {
            ::gamePacket gp{};
            gp.type = 0x13 | (3 << 24); // @note PACKET_ITEM_EFFECT, style 3 = trade
            gp.netid = P[1 - s]->netid; // @note to
            gp.uid = P[s]->netid;       // @note from
            gp.id = 150;                // @note flight time in ms
            gp.punch = ::pos{ static_cast<int>(o.id), static_cast<int>(o.id) };
            const ::blob fx = compress_state(gp);
            peers(P[0]->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { send_data(p, fx); });
        }

    auto list = [](const std::vector<offer> &l)
    {
        std::string s{};
        for (const offer &o : l) s += std::format("{}{} {}", s.empty() ? "" : ", ", o.count, id_to_item(static_cast<u_short>(o.id)).raw_name);
        return s.empty() ? std::string{ "nothing" } : s;
    };
    const std::string line = std::format("`1{}`` traded {} to `1{}`` for {}.", P[0]->growid, list(give[0]), P[1]->growid, list(give[1]));
    peers(P[0]->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { on::ConsoleMessage(&p, line); });
    for (int s = 0; s < 2; ++s)
    {
        send_varlist(pp[s], { "OnTextOverlay", "`2Trade complete!``" });
        send_action(*pp[s], "play_sfx", "file|audio/keypad_hit.wav\ndelayMS|0");
        reset_state(P[s]->user_id);
    }
}

/* my own window only (the other player hasn't opened theirs yet) */
void show_own(ENetPeer &p)
{
    ::peer *me = static_cast<::peer*>(p.data);
    send_status(p, *me, states[me->user_id], true, "\nlocked|0\nreset_locks|1\naccepted|0\n");
}

/* true while my trade window is open, whether or not the other side has joined */
bool window_open(const ::peer &me)
{
    auto it = states.find(me.user_id);
    return it != states.end() && it->second.partner != 0;
}

/* the partner of `me` if the window is live */
ENetPeer *partner_of(::peer &me)
{
    auto it = states.find(me.user_id);
    if (it == states.end() || !it->second.open) return nullptr;
    ENetPeer *p = peer_by_uid(me, it->second.partner);
    if (!p) return nullptr;
    auto other = states.find(static_cast<::peer*>(p->data)->user_id);
    if (other == states.end() || other->second.partner != me.user_id) return nullptr;
    return p;
}

void add_offer(ENetEvent &event, short id, int count)
{
    ::peer *me = me_of(event);
    if (!window_open(*me)) return;
    ENetPeer *other = partner_of(*me); // @note nullptr while the other player hasn't opened their window yet
    tstate &s = states[me->user_id];
    auto update = [&] { if (other) refresh(*event.peer, *other, true); else show_own(*event.peer); };

    count = std::min(count, owned(*me, id));
    if (count <= 0) { std::erase_if(s.items, [&](const offer &o) { return o.id == id; }); update(); return; }

    auto it = std::ranges::find(s.items, id, &offer::id);
    if (it == s.items.end())
    {
        if (s.items.size() >= 4) // @note wiki: 4 different stacks at most
        {
            send_varlist(event.peer, { "OnTextOverlay", "You can only trade 4 different items at once." });
            send_action(*event.peer, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
            return;
        }
        s.items.push_back({ id, count });
    }
    else it->count = count;
    update();
}
}

/* ---- starting ---- */

void trade_request(ENetEvent &event, ENetPeer &target)
{
    ::peer *me = me_of(event);
    ::peer *t = static_cast<::peer*>(target.data);

    if (me == t) { on::ConsoleMessage(event.peer, "`oYou trade all your stuff to yourself in exchange for all your stuff."); return; }
    if (me->netid == 0 || t->netid == 0 || me->recent_worlds.back() != t->recent_worlds.back()) { tell(event.peer, "`4You can only trade with players in the same world.``"); return; }
    if (me->curse_until > std::time(nullptr)) { tell(event.peer, "`4You can't trade while cursed.``"); return; }
    if (auto it = states.find(t->user_id); it != states.end() && it->second.open && it->second.partner != me->user_id)
    {
        send_varlist(event.peer, { "OnTalkBubble", me->netid, "`wThat person is busy.``", 0u });
        return;
    }
    if (auto mine = states.find(me->user_id); mine != states.end() && mine->second.open) close_for(event.peer, ""); // @note leave an old window first

    tstate &s = states[me->user_id];
    s = tstate{};
    s.partner = t->user_id;
    send_varlist(event.peer, { "OnStartTrade", name_of(*t), t->netid }); // @note the client opens the window and answers with trade_started
}

void trade_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *me = me_of(event);
    if (me->netid == 0) { tell(event.peer, "`4You must be in a world.``"); return; }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();
    if (arg.empty()) { tell(event.peer, "`4Usage: /trade {player}``"); return; }

    bool ambiguous = false;
    ENetPeer *found = find_in_world(me->recent_worlds.back(), arg, ambiguous);
    if (!found)
    {
        on::ConsoleMessage(event.peer, ambiguous ? "`4Oops:`` More than one player starts with that - type more letters."
                                                 : std::format("`4Oops:`` There is nobody currently in this world with a name starting with `w{}``.", arg));
        return;
    }
    trade_request(event, *found);
}

void trade_end_for(const ::peer &p, const std::string &why)
{
    for (auto &[uid, s] : states)
        if (s.partner == p.user_id && uid != p.user_id)
        {
            ENetPeer *other = nullptr;
            peers("", peer_condition::PEER_ALL, [&](ENetPeer &x) { const ::peer *t = static_cast<::peer*>(x.data); if (t && t->user_id == uid) other = &x; });
            if (other && s.open) close_for(other, std::format("`6[```4Trade canceled: `w{}`` {}`4!```6]``", p.growid, why));
            else if (other) reset_state(uid);
            break;
        }
    states.erase(p.user_id);
}

/* ---- client packets ---- */

void action::trade_started(ENetEvent &event, const std::string &header)
{
    ::peer *me = me_of(event);
    const int netid = std::atoi(::hPipe{ header }["netid"].c_str());
    auto mine = states.find(me->user_id);
    if (mine == states.end() || me->netid == 0) return;

    ENetPeer *other = nullptr;
    peers(me->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) { const ::peer *t = static_cast<::peer*>(p.data); if (t && t->netid == netid) other = &p; });
    if (!other) { close_for(event.peer, "`4That player left the world.``"); return; }
    ::peer *O = static_cast<::peer*>(other->data);

    auto theirs = states.find(O->user_id);
    if (theirs != states.end() && theirs->second.partner == me->user_id)
    {
        // both asked each other: the trade is live
        mine->second.open = theirs->second.open = true;
        refresh(*event.peer, *other, false);
        return;
    }

    on::ConsoleMessage(other, std::format("`#TRADE ALERT:`` {} `owants to trade with you!  To start, use the `wWrench`` on that person's wrench icon, or type `w/trade {}``", name_of(*me), me->growid));
    send_action(*other, "play_sfx", "file|audio/cash_register.wav\ndelayMS|0");
}

void action::mod_trade(ENetEvent &event, const std::string &header)
{
    ::peer *me = me_of(event);
    if (!window_open(*me)) return; // @note works before the other player has opened their window too

    const short id = static_cast<short>(std::atoi(::hPipe{ header }["itemID"].c_str()));
    std::string why{};
    if (!tradable(id, why))
    {
        send_varlist(event.peer, { "OnTextOverlay", why });
        send_action(*event.peer, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
        return;
    }
    tstate &s = states[me->user_id];
    const auto now = std::chrono::steady_clock::now();
    if (now - s.last_change < std::chrono::milliseconds(500))
    {
        send_varlist(event.peer, { "OnTextOverlay", "Slow down!  Please wait a second between adding and removing items" });
        return;
    }
    s.last_change = now;

    const int have = owned(*me, id);
    if (have <= 0) return;
    if (have == 1) { add_offer(event, id, 1); return; }

    const ::item &it = id_to_item(static_cast<u_short>(id));
    send_varlist(event.peer, { "OnDialogRequest", std::format(
        "set_default_color|`o\n"
        "add_label_with_icon|big|`2Trade`` `w{}``|left|{}|\n"
        "add_textbox|`2Trade how many?``|left|\n"
        "add_text_input|count||{}|5|\n"
        "embed_data|itemID|{}\n"
        "end_dialog|trade_add|Cancel|OK|\n", clean(it.raw_name), id, have, id) });
}

void action::rem_trade(ENetEvent &event, const std::string &header)
{
    ::peer *me = me_of(event);
    if (!window_open(*me)) return;
    const short id = static_cast<short>(std::atoi(::hPipe{ header }["itemID"].c_str()));
    std::erase_if(states[me->user_id].items, [&](const offer &o) { return o.id == id; });
    if (ENetPeer *other = partner_of(*me)) refresh(*event.peer, *other, true);
    else show_own(*event.peer);
}

void action::trade_accept(ENetEvent &event, const std::string &header)
{
    ::peer *me = me_of(event);
    ENetPeer *other = partner_of(*me);
    if (!other) return;
    ::peer *O = static_cast<::peer*>(other->data);
    tstate &sm = states[me->user_id], &so = states[O->user_id];

    sm.accepted = ::hPipe{ header }["status"] == "1";
    send_accepts(*event.peer, *other);
    if (!sm.accepted || !so.accepted) return;

    // both accepted: check both backpacks first
    short which = 0;
    for (auto [p, P, in, out] : { std::tuple{ event.peer, me, &so.items, &sm.items }, std::tuple{ other, O, &sm.items, &so.items } })
    {
        if (!fits(*P, *in, *out, which))
        {
            const std::string text = which
                ? std::format("`4Oops - {} `4is carrying too many {} and can't fit that many in their backpack.", name_of(*P), id_to_item(static_cast<u_short>(which)).raw_name)
                : std::format("`w{}`w needs more backpack room first!", name_of(*P));
            send_varlist(event.peer, { "OnTextOverlay", text });
            send_varlist(other, { "OnTextOverlay", text });
            sm.accepted = so.accepted = false;
            send_accepts(*event.peer, *other);
            return;
        }
    }

    // last step: both see what they give and get
    send_varlist(event.peer, { "OnForceTradeEnd" });
    send_varlist(other, { "OnForceTradeEnd" });
    confirm_dialog(*event.peer, sm, so);
    confirm_dialog(*other, so, sm);
}

void action::trade_cancel(ENetEvent &event, const std::string &header)
{
    ::peer *me = me_of(event);
    auto it = states.find(me->user_id);
    if (it == states.end()) return;
    ENetPeer *other = peer_by_uid(*me, it->second.partner);
    const bool was_open = it->second.open;
    reset_state(me->user_id);
    if (other && was_open)
    {
        auto theirs = states.find(static_cast<::peer*>(other->data)->user_id);
        if (theirs != states.end() && theirs->second.partner == me->user_id)
            close_for(other, std::format("{} `whas canceled the trade", name_of(*me)));
    }
}

/* ---- dialogs ---- */

void trade_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *me = me_of(event);
    const std::string dialog = hPipe["dialog_name"];

    if (dialog == "trade_add")
    {
        const short id = static_cast<short>(std::atoi(hPipe["itemID"].c_str()));
        const int count = std::atoi(hPipe["count"].c_str());
        if (count < 0) { send_varlist(event.peer, { "OnTextOverlay", "Whatever scammer. Bet you liked the game more before this cool trade system!" }); return; }
        std::string why{};
        if (count == 0 || !tradable(id, why)) return;
        add_offer(event, id, std::min(count, 200));
        return;
    }

    if (dialog != "trade_confirm") return;
    auto mine = states.find(me->user_id);
    if (mine == states.end() || !mine->second.open) { send_varlist(event.peer, { "OnTextOverlay", "The other person left the trade!" }); return; }
    ENetPeer *other = peer_by_uid(*me, mine->second.partner);
    auto theirs = other ? states.find(static_cast<::peer*>(other->data)->user_id) : states.end();

    if (hPipe["buttonClicked"] != "accept" || !other || theirs == states.end() || theirs->second.partner != me->user_id)
    {
        const std::string text = std::format("`6[```4Trade canceled by {}`4!```6]``", name_of(*me));
        close_for(event.peer, text);
        if (other && theirs != states.end() && theirs->second.partner == me->user_id)
        {
            send_varlist(other, { "OnDialogRequest", "set_default_color|`o\nadd_label_with_icon|big|`wTrade``|left|1366|\nadd_textbox|" + clean(text) + "|left|\nend_dialog|trade_done||OK|\n" });
            close_for(other, text);
        }
        return;
    }

    mine->second.confirmed = true;
    if (theirs->second.confirmed) execute(*event.peer, *other);
    else on::ConsoleMessage(event.peer, "`oWaiting for the other player to confirm...``");
}
