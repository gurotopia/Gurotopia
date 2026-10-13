#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "trade.hpp"
#include <charconv>

namespace
{
    using phase = ::peer::trade_state::phase;

    template<typename T>
    bool parse_number(const std::string &text, T &value)
    {
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    }

    bool valid_event(const ENetEvent &event)
    {
        if (!event.peer || !event.peer->data) return false;
        const ::peer &player = *static_cast<::peer*>(event.peer->data);
        return player.user_id > 0 && player.netid > 0;
    }

    ::block *world_lock(::world &world, ::pos &at)
    {
        for (std::size_t i = 0; i < world.blocks.size(); ++i)
        {
            const short id = world.blocks[i].fg;
            if (id <= 0 || id >= static_cast<int>(items.size())) continue;
            const ::item &item = id_to_item(id);
            if (item.type != type::LOCK || is_tile_lock(item.id)) continue;
            at = ::pos{static_cast<int>(i % 100), static_cast<int>(i / 100)};
            return &world.blocks[i];
        }
        return nullptr;
    }

    std::string world_display_name(const ::peer &player, const ::world &world)
    {
        const char *colour = player.user_id == world.owner ? "`2" :
            std::ranges::find(world.access, player.user_id) != world.access.end() ? "`c" : "`w";
        return std::format("{}{}``", colour, player.growid);
    }

    void rename_in_world(const ::peer &player)
    {
        peers(player.recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &connection) {
            send_varlist(&connection, { "OnNameChanged", player.display_growid }, player.netid);
        });
    }

    constexpr short kWorldKey = 1424;
    constexpr std::size_t kTradeSlots = 4;

    ::peer &of(ENetPeer &peer) { return *static_cast<::peer*>(peer.data); }

    /* @note netid can be reused; match the account as well. */
    ENetPeer *partner_of(const ::peer &peer)
    {
        if (peer.trade.partner_netid == 0 || peer.netid == 0) return nullptr;
        ENetPeer *found = nullptr;
        peers(peer.recent_worlds.back(), PEER_SAME_WORLD, [&found, &peer](ENetPeer &p)
        {
            const ::peer &o = of(p);
            if (o.netid == peer.trade.partner_netid && o.user_id == peer.trade.partner_uid) found = &p;
        });
        return found;
    }

    bool together(const ::peer &a, const ::peer &b)
    {
        return a.trade.partner_netid == b.netid && b.trade.partner_netid == a.netid &&
            a.trade.partner_uid == b.user_id && b.trade.partner_uid == a.user_id &&
            a.trade.started && b.trade.started;
    }

    short held(const ::peer &peer, short id)
    {
        const auto it = std::ranges::find(peer.slots, id, &::slot::id);
        return it == peer.slots.end() ? 0 : it->count;
    }

    bool gives_key(const ::peer &peer) { return std::ranges::find(peer.trade.offer, kWorldKey, &::slot::id) != peer.trade.offer.end(); }

    /* @note Accept changes accepted, not locked. Offer resets also need reset_locks. */
    void status(ENetPeer &to, const ::peer &whose, bool accepted, bool unticked = false)
    {
        std::string slots;
        for (const ::slot &slot : whose.trade.offer) slots.append(std::format("add_slot|{}|{}\n", slot.id, slot.count));
        send_varlist(&to, {
            "OnTradeStatus", whose.netid, "", std::format("`o{}'s offer.``", whose.display_growid),
            std::format("{}locked|0\n{}accepted|{}\n", slots, (accepted || unticked) ? "" : "reset_locks|1\n", accepted ? 1 : 0)
        });
    }

    void end_window(ENetPeer &peer)
    {
        send_varlist(&peer, { "OnForceTradeEnd" });
        of(peer).trade = {};
    }

    void over_head(ENetPeer &peer, const std::string &text)
    {
        send_varlist(&peer, { "OnTalkBubble", static_cast<u_int>(of(peer).netid), text, 0u, 0u });
        on::ConsoleMessage(&peer, text);
    }

    /* @note both clients need the tick/reset sequence when joining. */
    void joined(ENetPeer &joiner, ENetPeer &waiting)
    {
        ::peer &j = of(joiner), &w = of(waiting);
        j.trade.stage = w.trade.stage = phase::editing;
        j.trade.accepted = w.trade.accepted = j.trade.confirmed = w.trade.confirmed = false;
        status(joiner, j, false);
        status(joiner, j, true);
        status(joiner, j, false);
        status(joiner, w, false);
        status(waiting, w, true);
        status(waiting, w, false);
        status(waiting, j, false);
        status(waiting, w, false);
    }

    /* @note changing an offer clears both Accepts. */
    void changed(ENetPeer &peer, const std::string &line)
    {
        ::peer &x = of(peer);
        x.trade.accepted = x.trade.confirmed = false;
        status(peer, x, false);

        ENetPeer *other = partner_of(x);
        if (other == nullptr || !together(x, of(*other))) return;
        ::peer &y = of(*other);
        y.trade.accepted = y.trade.confirmed = false;
        send_varlist(&peer, { "OnTextOverlay", "The deal has changed" });
        send_action(peer, "play_sfx", "file|audio/tile_removed.wav\ndelayMS|0");
        if (!line.empty()) on::ConsoleMessage(other, line);
        send_varlist(other, { "OnTextOverlay", "The deal has changed" });
        send_action(*other, "play_sfx", "file|audio/tile_removed.wav\ndelayMS|0");
        status(*other, y, true);
        status(*other, y, false);
        status(*other, x, false);
    }

    std::string listing(const std::vector<::slot> &offer)
    {
        if (offer.empty()) return "add_textbox|`4Nothing!``|left|\n";
        std::string lines;
        for (const ::slot &slot : offer)
            lines.append(std::format("add_label_with_icon|small|(`w{}``) {}|left|{}|\n", slot.count, id_to_item(slot.id).raw_name, slot.id));
        return lines;
    }

    void confirm(ENetPeer &to, const ::peer &self, const ::peer &other, ::world &world)
    {
        std::string dialog = std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wTrade Confirmation``|left|1366|\n"
            "add_spacer|small|\n"
            "add_textbox|`4You'll give:``|left|\n"
            "add_spacer|small|\n"
            "{}"
            "add_spacer|small|\n"
            "add_textbox|`2You'll get:``|left|\n"
            "add_spacer|small|\n"
            "{}",
            listing(self.trade.offer), listing(other.trade.offer));
        if (gives_key(self))
            dialog.append(std::format(
                "add_spacer|small|\n"
                "add_textbox|`4WARNING: ``You are about to sell your world `#{}`` - the world lock ownership will be transferred over to {}.|left|\n",
                world.name, other.display_growid));
        if (other.trade.offer.empty())
            dialog.append(
                "add_spacer|small|\n"
                "add_spacer|small|\n"
                "add_textbox|`4SCAM WARNING: ``You are about to do a trade without receiving anything in return. Once you do the trade you cannot get the items back.|left|\n"
                "add_textbox|`4Do you really want to do this?``|left|\n");
        if (gives_key(other))
        {
            ::pos tile{};
            const ::block *lock = world_lock(world, tile);
            dialog.append(std::format(
                "add_spacer|small|\n"
                "add_textbox|`4SCAM WARNING: ``You are buying this world, `#{}``. Don't buy a world just to get its rare items, because scammers can easily place secret doors allowing someone to jump in and `4steal the items back`` after a trade!|left|\n"
                "add_spacer|small|\n"
                "add_textbox|Also, all ratings will be removed from a world when it is traded. Don't buy a world for its ranking!|left|\n"
                "add_spacer|small|\n"
                "add_textbox|To be safe, only buy a world for its name and placed blocks, not loose items or high ratings. Consider yourself warned!|left|\n"
                "add_spacer|small|\n"
                "add_textbox|This world is locked by {}.|left|\n",
                world.name, lock ? id_to_item(lock->fg).raw_name : "World Lock"));
        }
        dialog.append(
            "add_spacer|small|\n"
            "add_button|accept|Do The Trade!|noflags|0|0|\n"
            "add_button|back|Cancel|noflags|0|0|\n"
            "end_dialog|trade_confirm|||\n");
        send_varlist(&to, { "OnDialogRequest", dialog });
    }

    std::string items_text(const std::vector<::slot> &offer)
    {
        if (offer.empty()) return "nothing";
        std::string text;
        for (std::size_t i = 0; i < offer.size(); ++i)
        {
            if (i > 0) text.append(i + 1 == offer.size() ? " and " : ", ");
            text.append(std::format("{} {}", offer[i].count, id_to_item(offer[i].id).raw_name));
        }
        return text;
    }

    /* @note check the final backpack, including slots freed by outgoing items. */
    bool final_inventory(const ::peer &player, const std::vector<::slot> &incoming,
                         ::world &world, std::vector<::slot> &after)
    {
        after = player.slots;
        for (const ::slot &offered : player.trade.offer)
        {
            auto held = std::ranges::find(after, offered.id, &::slot::id);
            if (offered.id <= 0 || offered.id >= static_cast<int>(items.size()) ||
                offered.count <= 0 || offered.count > 200 || (id_to_item(offered.id).cat & CAT_UNTRADEABLE) ||
                held == after.end() || held->count < offered.count) return false;
            if (offered.id == kWorldKey)
            {
                ::pos tile{};
                if (offered.count != 1 || world.owner != player.user_id || !world_lock(world, tile)) return false;
            }
            held->count -= offered.count;
        }
        for (const ::slot &received : incoming)
        {
            if (received.id == kWorldKey) continue;
            auto held = std::ranges::find(after, received.id, &::slot::id);
            if (received.count <= 0 || received.count > 200 ||
                (held != after.end() && held->count + received.count > 200)) return false;
            if (held == after.end()) after.push_back(received);
            else held->count += received.count;
        }
        return std::ranges::count_if(after, [](const ::slot &slot) { return slot.count > 0; }) <= player.slot_size;
    }

    // Inventories are already updated; only send the client deltas here.
    void inventory_delta(ENetPeer &connection, const ::slot &change)
    {
        ::gamePacket packet{.id = change.id};
        packet.type = change.count < 0 ? (-change.count << 16) | 0x000d : (change.count << 24) | 0x000d;
        send_data(connection, compress_state(packet));
    }

    void sell_world(::world &world, ::peer &seller, ::peer &buyer)
    {
        ::pos tile{};
        const ::block *lock = world_lock(world, tile);
        if (lock == nullptr || world.owner != seller.user_id) return;

        std::vector<int> had_access;
        for (int &uid : world.access) if (uid != 0) had_access.push_back(uid), uid = 0;
        world.owner = buyer.user_id;
        std::ranges::replace(seller.my_worlds, world.name, std::string{});
        if (std::ranges::find(buyer.my_worlds, world.name) == buyer.my_worlds.end())
        {
            std::ranges::rotate(buyer.my_worlds, buyer.my_worlds.begin() + 1);
            buyer.my_worlds.back() = world.name;
        }

        const ::blob lock_packet = compress_state(::gamePacket{
            .type = 0x0f, // @note PACKET_SEND_LOCK: netID is the new owner's user_id
            .netid = buyer.user_id,
            .state = state::S_EXTENDED,
            .id = lock->fg,
            .punch = tile
        });
        peers(world.name, PEER_SAME_WORLD, [&](ENetPeer &connection) { send_data(connection, lock_packet); });
        const std::string line = std::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world.name, buyer.growid);
        peers(world.name, PEER_SAME_WORLD, [&line, &buyer](ENetPeer &p)
        {
            send_action(p, "play_sfx", "file|audio/use_lock.wav\ndelayMS|0");
            send_varlist(&p, { "OnTalkBubble", static_cast<u_int>(buyer.netid), line, 0u });
            on::ConsoleMessage(&p, line);
        });
        buyer.display_growid = world_display_name(buyer, world);
        rename_in_world(buyer);
        seller.display_growid = world_display_name(seller, world);
        rename_in_world(seller);
        peers(world.name, PEER_SAME_WORLD, [&world, &had_access, &buyer](ENetPeer &p)
        {
            ::peer &o = of(p);
            if (&o == &buyer || std::ranges::find(had_access, o.user_id) == had_access.end()) return;
            o.display_growid = world_display_name(o, world);
            rename_in_world(o);
        });
    }

    /* @note effects start with the last confirmer, 350 ms apart. */
    void execute(ENetPeer &last, ENetPeer &first)
    {
        ::peer &a = of(last), &b = of(first);
        auto world = std::ranges::find(worlds, a.recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;

        std::vector<::slot> final_a, final_b;
        if (!final_inventory(a, b.trade.offer, *world, final_a) || !final_inventory(b, a.trade.offer, *world, final_b))
        {

            for (ENetPeer *p : {&last, &first})
            {
                send_varlist(p, { "OnTextOverlay", "`4The trade could not be completed.``" });
                end_window(*p);
            }
            return;
        }
        // Apply both backpacks before sending the trade packets.
        for (auto [player, inventory] : { std::pair{&a, &final_a}, std::pair{&b, &final_b} })
        {
            for (const ::slot &offered : player->trade.offer)
                if (held(*player, offered.id) == offered.count)
                {
                    const auto kind = id_to_item(offered.id).cloth_type;
                    if (kind != clothing::NONE && player->clothing[kind] == offered.id) player->clothing[kind] = 0;
                }
            player->slots = std::move(*inventory);
            player->update_effects();
        }
        std::printf("Trade in %s: %s gave %s; %s gave %s\n", world->name.c_str(), a.growid.c_str(),
                    items_text(a.trade.offer).c_str(), b.growid.c_str(), items_text(b.trade.offer).c_str());

        int delay = 0;
        for (auto [giver, getter] : { std::pair{&last, &first}, std::pair{&first, &last} })
        {
            ::peer &g = of(*giver), &r = of(*getter);
            const std::string giver_name = g.display_growid; // @note keep the seller's old name for the trade line
            const std::vector<::slot> offer = g.trade.offer;
            for (std::size_t i = 0; i < offer.size(); ++i)
            {
                const ::slot &slot = offer[i];
                const int at = delay + 300 + 350 * static_cast<int>(i);
                /* @note ITEM_EFFECT: receiver netid, giver uid, delay and item id. */
                const ::blob flight = compress_state(::gamePacket{ .type = 0x03000013, .netid = r.netid, .uid = g.netid, .id = at, .punch = ::pos{slot.id, slot.id} });
                for (ENetPeer *p : {&last, &first})
                {
                    send_data(*p, flight);
                    send_varlist(p, { "OnPlayPositioned", "audio/change_clothes.wav" }, g.netid, at);
                }
                inventory_delta(*giver, ::slot{slot.id, static_cast<short>(-slot.count)});
                if (slot.id != kWorldKey) inventory_delta(*getter, slot); // @note the key is spent: the buyer gets the world, not the key
            }
            if (gives_key(g)) sell_world(*world, g, r);

            delay += 300 + 350 * static_cast<int>(offer.size());
            const std::string line = std::format("`1{} traded {} to {}.``", giver_name, items_text(offer), r.growid);
            on::ConsoleMessage(giver, line, delay);
            on::ConsoleMessage(getter, line, delay);
        }
        for (ENetPeer *p : {&last, &first})
            send_varlist(p, { "OnPlayPositioned", "audio/keypad_hit.wav" }, a.netid, delay + 300);
        end_window(last);
        end_window(first);
    }

    void cancel_confirm(ENetPeer &peer, ENetPeer *other)
    {
        const std::string line = std::format("`6[```4Trade canceled by`w {}``!```6]``", of(peer).growid);
        for (ENetPeer *p : {&peer, other})
        {
            if (p == nullptr) continue;
            over_head(*p, line);
            send_varlist(p, { "OnTextOverlay", line });
            end_window(*p);
        }
    }

    void close_window(ENetPeer &peer)
    {
        ::peer &x = of(peer);
        if (ENetPeer *other = partner_of(x); other != nullptr && of(*other).trade.partner_netid == x.netid)
        {
            send_varlist(other, { "OnTextOverlay", std::format("{} has canceled the trade", x.display_growid) });
            end_window(*other);
        }
        x.trade = {};
    }

    /* @note count replaces the offer; 0 removes it. */
    void offer(ENetPeer &peer, short id, int count)
    {
        ::peer &x = of(peer);
        auto it = std::ranges::find(x.trade.offer, id, &::slot::id);
        count = std::min(count, static_cast<int>(held(x, id)));
        if (count <= 0)
        {
            if (it == x.trade.offer.end()) return;
            x.trade.offer.erase(it);
            changed(peer, "");
            return;
        }
        if (it == x.trade.offer.end())
        {
            if (x.trade.offer.size() >= kTradeSlots) return;
            x.trade.offer.emplace_back(id, static_cast<short>(count));
        }
        else it->count = static_cast<short>(count);
        changed(peer, std::format("`1TRADE CHANGE: `` {} added `w{}`` {}", x.growid, count, id_to_item(id).raw_name));
    }
}

void trade_request(ENetEvent& event, ENetPeer &connection)
{
    if (!valid_event(event)) return;
    ::peer &x = of(*event.peer);
    if (!connection.data || connection.state != ENET_PEER_STATE_CONNECTED) return;
    ENetPeer *target = &connection;
    ::peer &t = of(*target);
    if (t.user_id <= 0 || t.user_id == x.user_id || t.netid == 0 || x.recent_worlds.back() != t.recent_worlds.back()) return;
    if (t.trade.stage != phase::idle && t.trade.stage != phase::waiting && t.trade.partner_uid != x.user_id) return;
    if (x.trade.partner_uid == t.user_id && x.trade.partner_netid == t.netid) return;

    if (x.trade.partner_netid != 0) close_window(*event.peer);
    x.trade = {};
    x.trade.stage = phase::waiting;
    x.trade.partner_netid = t.netid;
    x.trade.partner_uid = t.user_id;

    /* @note OnStartTrade takes the bare GrowID. */
    send_varlist(event.peer, { "OnStartTrade", t.growid, t.netid });
    if (t.trade.partner_netid != x.netid)
    {
        on::ConsoleMessage(target, std::format(
            "`#TRADE ALERT:`` `w{0}`` wants to trade with you!  To start, use the `wWrench`` on that person's wrench icon, or type `w/trade {0}",
            x.display_growid));
        send_action(*target, "play_sfx", "file|audio/cash_register.wav\ndelayMS|0");
    }
}

void trade_partner_left(ENetPeer& other, const ::peer &leaver)
{
    ::peer &o = of(other);
    if (leaver.netid == 0 || o.trade.partner_netid != leaver.netid || o.trade.partner_uid != leaver.user_id) return;
    send_varlist(&other, { "OnTextOverlay", std::format("{} has canceled the trade", leaver.display_growid) });
    end_window(other);
}

void action::trade_started(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;
    ::peer &x = of(*event.peer);
    int netid{};
    if (x.trade.stage != phase::waiting || x.trade.started || !parse_number(::hPipe{header}["netid"], netid) || netid != x.trade.partner_netid) return;
    x.trade.started = true;

    ENetPeer *other = partner_of(x);
    if (other != nullptr && together(x, of(*other))) joined(*event.peer, *other);
}

void action::mod_trade(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;
    ::peer &x = of(*event.peer);
    if (x.trade.stage == phase::idle || x.trade.stage == phase::confirming || !x.trade.started) return;
    short id{};
    if (!parse_number(::hPipe{header}["itemID"], id) || id <= 0 || id >= static_cast<int>(items.size()) || held(x, id) <= 0) return;
    const ::item &item = id_to_item(id);
    if (item.cat & CAT_UNTRADEABLE)
    {
        send_varlist(event.peer, { "OnTextOverlay", "You'd be sorry if you lost that!" });
        send_action(*event.peer, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
        return;
    }
    if (id == kWorldKey)
    {
        /* @note only the current world owner may offer its key. */
        auto world = std::ranges::find(worlds, x.recent_worlds.back(), &::world::name);
        if (world == worlds.end() || world->owner != x.user_id) return;
    }

    /* @note single items go straight in; stacks ask for a quantity. */
    const short count = held(x, static_cast<short>(id));
    if (count == 1)
    {
        offer(*event.peer, static_cast<short>(id), 1);
        return;
    }
    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`2Trade`` `w{}``|left|{}|\n"
            "add_textbox|`2Trade how many?``|left|\n"
            "add_text_input|count||{}|5|\n"
            "embed_data|itemID|{}\n"
            "end_dialog|trade_item|Cancel|OK|\n",
            item.raw_name, id, count, id
        )
    });
}

void action::rem_trade(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;

    const ::peer &player = of(*event.peer);
    short id{};
    if (player.trade.started && player.trade.stage != phase::confirming && parse_number(::hPipe{header}["itemID"], id) && id > 0)
        offer(*event.peer, id, 0);
}

void trade_item(ENetEvent& event, const ::hPipe &hPipe)
{
    if (!valid_event(event)) return;
    const ::peer &player = of(*event.peer);
    short id{};
    int count{};
    if (!player.trade.started || player.trade.stage == phase::confirming ||
        !parse_number(hPipe["itemID"], id) || !parse_number(hPipe["count"], count) || count < 0 ||
        id <= 0 || id >= static_cast<int>(items.size()) || (id_to_item(id).cat & CAT_UNTRADEABLE)) return;
    if (id == kWorldKey)
    {
        auto world = std::ranges::find(worlds, player.recent_worlds.back(), &::world::name);
        if (world == worlds.end() || world->owner != player.user_id) return;
        count = std::min(count, 1);
    }
    offer(*event.peer, id, std::min(count, 200));
}

void action::trade_accept(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;
    ::peer &x = of(*event.peer);
    ENetPeer *other = partner_of(x);
    if (other == nullptr || !together(x, of(*other)) || x.trade.stage != phase::editing) return;
    ::peer &y = of(*other);

    /* @note the client must accept the current offer, not an older one. */
    ::hPipe hPipe{header};
    std::vector<std::string> pipes = readch(header, '|');
    std::vector<::slot> seen;
    if (hPipe["status"] != "0" && hPipe["status"] != "1") return;
    for (std::size_t i = 0; i < pipes.size(); ++i)
        if (pipes[i] == "item")
        {
            short id{}, count{};
            if (i + 2 >= pipes.size() || seen.size() >= kTradeSlots ||
                !parse_number(pipes[i + 1], id) || !parse_number(pipes[i + 2], count)) return;
            seen.emplace_back(id, count);
            i += 2;
        }
    if (hPipe["status"] == "1" && !std::ranges::equal(seen, y.trade.offer, [](const ::slot &a, const ::slot &b) { return a.id == b.id && a.count == b.count; }))
        return;

    x.trade.accepted = hPipe["status"] == "1";
    if (x.trade.accepted && y.trade.accepted)
    {
        auto world = std::ranges::find(worlds, x.recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;
        x.trade.stage = y.trade.stage = phase::confirming;
        x.trade.confirmed = y.trade.confirmed = false;

        for (auto [to, self, them] : { std::tuple{event.peer, &x, &y}, std::tuple{other, &y, &x} })
        {
            status(*to, x, true);
            send_varlist(to, { "OnForceTradeEnd" });
            confirm(*to, *self, *them, *world);
        }
        return;
    }
    /* @note reset the other side before sending the changed Accept state. */
    status(*event.peer, x, x.trade.accepted, !x.trade.accepted);
    status(*other, y, true);
    status(*other, y, false);
    status(*other, x, x.trade.accepted, !x.trade.accepted);
}

void action::trade_cancel(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;
    close_window(*event.peer);
}

void action::trade_check_price(ENetEvent& event, const std::string& header)
{
    if (!valid_event(event)) return;
    /* @todo Trade-Scan pricing data. */
    send_varlist(event.peer, {
        "OnDialogRequest",
        "set_default_color|`o\n"
        "add_label_with_icon|big|`wTrade-Scan``|left|13816|\n"
        "add_spacer|small|\n"
        "add_textbox|No items to check the pricing for!|left|\n"
        "end_dialog|p2p_trade_price_check_items_ui|Back||\n"
        "add_quick_exit|\n"
    });
}

void trade_confirm(ENetEvent& event, const ::hPipe &hPipe)
{
    if (!valid_event(event)) return;
    ::peer &x = of(*event.peer);
    ENetPeer *other = partner_of(x);
    if (other == nullptr || !together(x, of(*other)) || !x.trade.accepted || !of(*other).trade.accepted ||
        x.trade.stage != phase::confirming || of(*other).trade.stage != phase::confirming)
    {
        send_varlist(event.peer, { "OnTextOverlay", "The other person left the trade!" }); // @note
        x.trade = {};
        return;
    }
    ::peer &y = of(*other);

    if (hPipe["buttonClicked"] == "back")
    {
        cancel_confirm(*event.peer, other);
        return;
    }
    if (hPipe["buttonClicked"] != "accept") return;

    /* @note the seller must confirm before the buyer. */
    if (gives_key(y) && !y.trade.confirmed)
    {
        auto world = std::ranges::find(worlds, x.recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;
        send_varlist(event.peer, { "OnTextOverlay", "`6[``you must wait for the other player to accept first`6]``" });
        send_varlist(event.peer, { "OnForceTradeEnd" });
        confirm(*event.peer, x, y, *world);
        return;
    }
    if (!y.trade.confirmed)
    {
        x.trade.confirmed = true;
        over_head(*event.peer, "`6[``Trade accepted, waiting for other player to accept`6]``");
        over_head(*other, "`6[``Trade accepted by other player, waiting for you`6]``");
        return;
    }
    execute(*event.peer, *other);
}

void trade_end_for(ENetPeer &connection)
{
    if (!connection.data) return;
    const ::peer &player = of(connection);
    peers("", PEER_ALL, [&](ENetPeer &other) {
        if (&other != &connection && other.data) trade_partner_left(other, player);
    });
    of(connection).trade = {};
}

void trade_return(ENetEvent &event, const ::hPipe &packet)
{
    if (packet["dialog_name"] == "trade_item" || packet["dialog_name"] == "trade_add") trade_item(event, packet);
    else if (packet["dialog_name"] == "trade_confirm") trade_confirm(event, packet);
}

void trade_world_key(ENetEvent &event)
{
    if (!valid_event(event)) return;
    ::peer &player = of(*event.peer);
    auto world = std::ranges::find(worlds, player.recent_worlds.back(), &::world::name);
    ::pos tile{};
    if (world == worlds.end() || world->owner != player.user_id || !world_lock(*world, tile) || held(player, kWorldKey) > 0) return;
    auto bubble = [&](const std::string &text) {
        send_varlist(event.peer, { "OnTalkBubble", static_cast<u_int>(player.netid), text, 0u, 0u });
    };
    if (std::ranges::any_of(world->access, [](int uid) { return uid != 0; }))
    {
        bubble("You'll first need to `4remove all co-owners`` from your `5World Lock`` to get a `#World Key`` to trade this world.");
        return;
    }
    if (!world->objects.empty())
    {
        bubble("`4Oops!... Can't trade a world with floating items that are unblocked or in treasure chests!``");
        return;
    }
    for (const ::block &block : world->blocks)
        for (short id : {block.fg, block.bg})
        {
            if (id < 0 || id >= static_cast<int>(items.size())) return;
            if (id && (id_to_item(id).cat & CAT_UNTRADEABLE))
            {
                bubble("`4Oops!... Can't trade a world with Untradeable blocks in it!``");
                return;
            }
        }
    if (std::ranges::count_if(player.slots, [](const ::slot &slot) { return slot.count > 0; }) >= player.slot_size)
    {
        bubble("You need more backpack room first!");
        return;
    }
    bubble("You got a `#World Key``! You can now trade this world to other players.");
    modify_item_inventory(event, ::slot{kWorldKey, 1});
    send_action(*event.peer, "play_sfx", "file|audio/use_lock.wav\ndelayMS|0");
}
