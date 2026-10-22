#include "pch.hpp"
#include "trade.hpp"
#include "trade_internal.hpp"

namespace trade
{
    ::peer &of(ENetPeer &connection) { return *static_cast<::peer*>(connection.data); }

    bool valid_event(const ENetEvent &event)
    {
        if (!event.peer || !event.peer->data) return false;
        const ::peer &player = *static_cast<::peer*>(event.peer->data);
        return player.user_id > 0 && player.netid > 0;
    }

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

    bool valid_item(short id)
    {
        return id > 0 && id < static_cast<int>(items.size());
    }

    bool tradable_item(short id)
    {
        if (!valid_item(id)) return false;
        return !(id_to_item(id).cat & CAT_UNTRADEABLE);
    }

    bool may_edit(const ::peer &player)
    {
        return player.trade.started && player.trade.stage != phase::idle && player.trade.stage != phase::confirming;
    }

    int used_slots(const std::vector<::slot> &inventory)
    {
        return std::ranges::count_if(inventory, [](const ::slot &slot) { return slot.count > 0; });
    }

    bool gives_key(const ::peer &player)
    {
        return std::ranges::find(player.trade.offer, kWorldKey, &::slot::id) != player.trade.offer.end();
    }

    ::world *current_world(const ::peer &player)
    {
        auto world = std::ranges::find(worlds, player.recent_worlds.back(), &::world::name);
        return world == worlds.end() ? nullptr : &*world;
    }

    bool same_world(const ::peer &player, const ::peer &target)
    {
        return target.user_id > 0 && target.user_id != player.user_id && target.netid > 0 &&
            target.recent_worlds.back() == player.recent_worlds.back();
    }

    bool busy_with_other(const ::peer &target, int user_id)
    {
        return target.trade.stage != phase::idle && target.trade.stage != phase::waiting &&
            target.trade.partner_uid != user_id;
    }

    bool can_request(const ::peer &player, const ENetPeer &connection)
    {
        if (!connection.data || connection.state != ENET_PEER_STATE_CONNECTED) return false;
        const ::peer &target = *static_cast<::peer*>(connection.data);
        if (!same_world(player, target)) return false;
        return !busy_with_other(target, player.user_id);
    }

    bool requested(const ::peer &player, const ::peer &target)
    {
        return player.trade.partner_uid == target.user_id && player.trade.partner_netid == target.netid;
    }

    bool owned_item(const ::hPipe &packet, const ::peer &player, short &id)
    {
        if (!parse_number(packet["itemID"], id)) return false;
        if (!valid_item(id)) return false;
        return held(player, id) > 0;
    }

    bool select_item(ENetPeer &connection, short id)
    {
        if (!tradable_item(id))
        {
            send_varlist(&connection, { "OnTextOverlay", "You'd be sorry if you lost that!" });
            send_action(connection, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
            return false;
        }
        return can_offer_key(of(connection), id);
    }

    bool parse_offer(const ::hPipe &packet, short &id, int &count)
    {
        if (!parse_number(packet["itemID"], id)) return false;
        if (!parse_number(packet["count"], count)) return false;
        return count >= 0;
    }

    bool read_seen_item(const std::vector<std::string> &fields, std::size_t index, ::slot &item)
    {
        if (index + 2 >= fields.size()) return false;
        return parse_number(fields[index + 1], item.id) && parse_number(fields[index + 2], item.count);
    }

    bool read_seen_offer(const std::string &header, std::vector<::slot> &seen)
    {
        const std::vector<std::string> fields = readch(header, '|');
        for (std::size_t i = 0; i < fields.size(); ++i)
        {
            if (fields[i] != "item") continue;
            if (seen.size() >= kTradeSlots) return false;
            ::slot item;
            if (!read_seen_item(fields, i, item)) return false;
            seen.push_back(item);
            i += 2;
        }
        return true;
    }

    bool same_item(const ::slot &first, const ::slot &second)
    {
        return first.id == second.id && first.count == second.count;
    }

    /* @note Accept must describe the offer currently shown by the other player. */
    bool current_offer(const ::hPipe &packet, const std::string &header, const ::peer &partner)
    {
        const std::string status = packet["status"];
        if (status != "0" && status != "1") return false;
        std::vector<::slot> seen;
        if (!read_seen_offer(header, seen)) return false;
        if (status == "0") return true;
        return std::ranges::equal(seen, partner.trade.offer, same_item);
    }

    bool can_accept(const ::peer &player, const ::peer &partner)
    {
        return together(player, partner) && player.trade.stage == phase::editing;
    }

    bool confirming(const ::peer &first, const ::peer &second)
    {
        if (!together(first, second)) return false;
        return first.trade.accepted && second.trade.accepted &&
            first.trade.stage == phase::confirming && second.trade.stage == phase::confirming;
    }

    ENetPeer *confirmation_partner(const ::peer &player)
    {
        ENetPeer *partner = partner_of(player);
        if (!partner) return nullptr;
        return confirming(player, of(*partner)) ? partner : nullptr;
    }
}

void trade_request(ENetEvent &event, ENetPeer &connection)
{
    if (!trade::valid_event(event)) return;
    ::peer &player = trade::of(*event.peer);
    if (!trade::can_request(player, connection)) return;
    const ::peer &target = trade::of(connection);
    if (trade::requested(player, target)) return;
    if (player.trade.partner_netid != 0) trade::close_window(*event.peer);
    player.trade = {};
    player.trade.stage = trade::phase::waiting;
    player.trade.partner_netid = target.netid;
    player.trade.partner_uid = target.user_id;
    trade::announce_request(*event.peer, connection);
}

void trade_partner_left(ENetPeer &other, const ::peer &leaver)
{
    ::peer &player = trade::of(other);
    if (leaver.netid == 0 || player.trade.partner_netid != leaver.netid || player.trade.partner_uid != leaver.user_id) return;
    send_varlist(&other, { "OnTextOverlay", std::format("{} has canceled the trade", leaver.display_growid) });
    trade::end_window(other);
}

void action::trade_started(ENetEvent &event, const std::string &header)
{
    if (!trade::valid_event(event)) return;
    ::peer &player = trade::of(*event.peer);
    int netid{};
    if (player.trade.stage != trade::phase::waiting || player.trade.started ||
        !trade::parse_number(::hPipe{header}["netid"], netid) || netid != player.trade.partner_netid) return;
    player.trade.started = true;
    ENetPeer *other = trade::partner_of(player);
    if (other != nullptr && trade::together(player, trade::of(*other))) trade::joined(*event.peer, *other);
}

void action::mod_trade(ENetEvent &event, const std::string &header)
{
    if (!trade::valid_event(event)) return;
    const ::peer &player = trade::of(*event.peer);
    if (!trade::may_edit(player)) return;
    short id{};
    if (!trade::owned_item(::hPipe{header}, player, id)) return;
    if (!trade::select_item(*event.peer, id)) return;
    const short count = trade::held(player, id);
    if (count == 1) { trade::offer(*event.peer, id, 1); return; }
    trade::quantity_dialog(*event.peer, id, count);
}

void action::rem_trade(ENetEvent &event, const std::string &header)
{
    if (!trade::valid_event(event)) return;
    const ::peer &player = trade::of(*event.peer);
    short id{};
    if (trade::may_edit(player) && trade::parse_number(::hPipe{header}["itemID"], id) && id > 0)
        trade::offer(*event.peer, id, 0);
}

void trade_item(ENetEvent &event, const ::hPipe &packet)
{
    if (!trade::valid_event(event)) return;
    const ::peer &player = trade::of(*event.peer);
    if (!trade::may_edit(player)) return;
    short id{};
    int count{};
    if (!trade::parse_offer(packet, id, count)) return;
    if (!trade::tradable_item(id)) return;
    if (!trade::can_offer_key(player, id)) return;
    const int limit = id == trade::kWorldKey ? 1 : 200;
    trade::offer(*event.peer, id, std::min(count, limit));
}

void action::trade_accept(ENetEvent &event, const std::string &header)
{
    if (!trade::valid_event(event)) return;
    ::peer &player = trade::of(*event.peer);
    ENetPeer *other = trade::partner_of(player);
    if (!other) return;
    ::peer &partner = trade::of(*other);
    if (!trade::can_accept(player, partner)) return;
    const ::hPipe packet{header};
    if (!trade::current_offer(packet, header, partner)) return;
    player.trade.accepted = packet["status"] == "1";
    if (player.trade.accepted && partner.trade.accepted)
    {
        trade::open_confirmation(*event.peer, *other);
        return;
    }
    trade::send_accept(*event.peer, *other);
}

void action::trade_cancel(ENetEvent &event, const std::string &header)
{
    if (trade::valid_event(event)) trade::close_window(*event.peer);
}

void action::trade_check_price(ENetEvent &event, const std::string &header)
{
    if (trade::valid_event(event)) trade::price_dialog(*event.peer);
}

void trade_confirm(ENetEvent &event, const ::hPipe &packet)
{
    if (!trade::valid_event(event)) return;
    ::peer &player = trade::of(*event.peer);
    ENetPeer *other = trade::confirmation_partner(player);
    if (!other)
    {
        send_varlist(event.peer, { "OnTextOverlay", "The other person left the trade!" });
        player.trade = {};
        return;
    }
    if (packet["buttonClicked"] == "back") { trade::cancel_confirm(*event.peer, other); return; }
    if (packet["buttonClicked"] != "accept") return;
    const ::peer &partner = trade::of(*other);
    if (trade::wait_for_seller(*event.peer, partner)) return;
    if (!partner.trade.confirmed) { trade::notify_waiting(*event.peer, *other); return; }
    trade::execute(*event.peer, *other);
}

void trade_end_for(ENetPeer &connection)
{
    if (!connection.data) return;
    const ::peer &player = trade::of(connection);
    peers("", PEER_ALL, [&](ENetPeer &other) {
        if (&other != &connection && other.data) trade_partner_left(other, player);
    });
    trade::of(connection).trade = {};
}

void trade_return(ENetEvent &event, const ::hPipe &packet)
{
    if (packet["dialog_name"] == "trade_item" || packet["dialog_name"] == "trade_add") trade_item(event, packet);
    else if (packet["dialog_name"] == "trade_confirm") trade_confirm(event, packet);
}
