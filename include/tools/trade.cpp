#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"

#include "trade.hpp"

ENetPeer *trade_target(const ::peer &pPeer)
{
    if (pPeer.netid == 0 || pPeer.trade.netid == 0) return nullptr;

    for (ENetPeer *peer : peers(pPeer.recent_worlds.back(), PEER_SAME_WORLD))
        if (static_cast<::peer*>(peer->data)->netid == pPeer.trade.netid) return peer;

    return nullptr;
}

ENetPeer *trade_partner(const ::peer &pPeer)
{
    ENetPeer *target = trade_target(pPeer);
    if (target == nullptr || static_cast<::peer*>(target->data)->trade.netid != pPeer.netid) return nullptr;

    return target;
}

void trade_request(ENetEvent &event, ENetPeer &target)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    ::peer *pOthers = static_cast<::peer*>(target.data);
    if (pPeer->netid == 0) return; // @note not in a world

    if (pOthers->user_id == pPeer->user_id)
    {
        on::ConsoleMessage(event.peer, "`oYou trade all your stuff to yourself in exchange for all your stuff.");
        return;
    }
    if (ENetPeer *busy = trade_partner(*pOthers); busy != nullptr && busy != event.peer)
    {
        send_varlist(event.peer, { "OnTalkBubble", pPeer->netid, "`wThat person is busy.``", 0u });
        return;
    }
    if (ENetPeer *partner = trade_partner(*pPeer); partner != nullptr) // @note leave the trade we are already in
        trade_end(*partner, std::format("{} `whas canceled the trade", pPeer->display_growid));

    pPeer->trade = { .netid = pOthers->netid };
    send_varlist(event.peer, { "OnStartTrade", pOthers->display_growid, pOthers->netid }); // @note the client opens the window and answers with action|trade_started
}

bool tradeable(short id, std::string &why)
{
    if (id <= 0 || static_cast<std::size_t>(id) >= items.size())
    {
        why = "That's not an item.";
        return false;
    }
    const ::item &item = id_to_item(id);

    if (item.type == type::FIST || item.type == type::WRENCH)
    {
        why = "You'd be sorry if you lost that!";
        return false;
    }
    const bool wand = (id == 274 || id == 276 || id == 278 || id == 732); // @note Freeze, Fire, Curse and Ban Wand are untradeable but can still be traded
    if (item.cat & CAT_UNTRADEABLE && !wand)
    {
        why = "That item can't be traded.";
        return false;
    }
    return true;
}

void trade_offer(ENetEvent &event, short id, short count)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->trade.netid == 0) return; // @note trade window isn't open

    auto slot = std::ranges::find(pPeer->slots, id, &::slot::id);
    count = std::min(count, (slot == pPeer->slots.end()) ? short{0} : slot->count); // @note can't offer more than we have

    std::vector<::slot> &offer = pPeer->trade.offer;
    auto it = std::ranges::find(offer, id, &::slot::id);

    if (count <= 0)
    {
        if (it != offer.end()) offer.erase(it);
    }
    else if (it != offer.end()) it->count = count;
    else if (offer.size() >= 4) // @note 4 different items at most
    {
        send_varlist(event.peer, { "OnTextOverlay", "You can only trade 4 different items at once." });
        send_action(*event.peer, "play_sfx", "file|audio/cant_place_tile.wav\ndelayMS|0");
        return;
    }
    else offer.emplace_back(id, count);

    trade_update(*event.peer, true);
}

void trade_status(ENetPeer &to, const ::peer &owner, const std::string &flags)
{
    const bool mine = static_cast<::peer*>(to.data)->user_id == owner.user_id;

    std::string slots{};
    for (const ::slot &slot : owner.trade.offer)
        slots.append(std::format("add_slot|{}|{}\n", slot.id, slot.count));

    send_varlist(&to, {
        "OnTradeStatus",
        owner.netid,
        "",
        mine ? "`oSelect an item from the inventory.``" : std::format("`w{}``'s offer.``", owner.display_growid),
        slots + flags
    });
}

void trade_update(ENetPeer &peer, bool deal_changed)
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);
    const std::string flags = "\nlocked|0\nreset_locks|1\naccepted|0\n";

    pPeer->trade.accepted = pPeer->trade.confirmed = false;
    trade_status(peer, *pPeer, flags);

    ENetPeer *other = trade_partner(*pPeer);
    if (other == nullptr) return; // @note the other peer hasn't opened their window yet
    ::peer *pOthers = static_cast<::peer*>(other->data);

    pOthers->trade.accepted = pOthers->trade.confirmed = false;
    trade_status(peer, *pOthers, flags);
    trade_status(*other, *pOthers, flags);
    trade_status(*other, *pPeer, flags);

    if (deal_changed)
        for (ENetPeer *p : { &peer, other })
        {
            send_varlist(p, { "OnTextOverlay", "The deal has changed" });
            send_action(*p, "play_sfx", "file|audio/tile_removed.wav\ndelayMS|0");
        }
}

void trade_end(ENetPeer &peer, const std::string &message)
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);

    send_varlist(&peer, { "OnForceTradeEnd" });
    if (!message.empty())
    {
        on::ConsoleMessage(&peer, message);
        send_varlist(&peer, { "OnTextOverlay", message });
    }
    pPeer->trade = {};
}

void trade_execute(ENetPeer &peer, ENetPeer &other)
{
    const std::array<ENetPeer*, 2ull> traders{ &peer, &other };
    const std::array<::peer*, 2ull> pTraders{ static_cast<::peer*>(peer.data), static_cast<::peer*>(other.data) };

    for (const ::peer *pTrader : pTraders)
        for (const ::slot &slot : pTrader->trade.offer)
        {
            auto have = std::ranges::find(pTrader->slots, slot.id, &::slot::id);
            if (have == pTrader->slots.end() || have->count < slot.count) // @note item disappeared mid-trade
            {
                for (ENetPeer *p : traders) trade_end(*p, "`4The trade failed:`` an item isn't there anymore.");
                return;
            }
        }

    const std::array<std::vector<::slot>, 2ull> give{ pTraders[0]->trade.offer, pTraders[1]->trade.offer };
    for (std::size_t i = 0; i < 2; ++i)
    {
        ENetEvent event{};
        event.peer = traders[i]; // @note modify_item_inventory() works on an event
        for (const ::slot &slot : give[i])     modify_item_inventory(event, ::slot(slot.id, -slot.count));
        for (const ::slot &slot : give[1 - i]) modify_item_inventory(event, slot);

        if (std::ranges::any_of(give[i], [](const ::slot &slot) { return id_to_item(slot.id).cloth_type != clothing::NONE; }))
        {
            pTraders[i]->update_effects(); // @note might have traded away something they were wearing
            on::SetClothing(*traders[i]);
        }
    }

    const std::string &world = pTraders[0]->recent_worlds.back();
    for (std::size_t i = 0; i < 2; ++i)
        for (const ::slot &slot : give[i])
        {
            const ::blob effect = compress_state(::gamePacket{
                .type  = 0x13 | (3 << 24), // @note PACKET_ITEM_EFFECT, style 3 (trade)
                .netid = pTraders[1 - i]->netid, // @note to
                .uid   = pTraders[i]->netid, // @note from
                .id    = 150, // @note flight time in ms
                .punch = ::pos{ static_cast<int>(slot.id), static_cast<int>(slot.id) }
            });
            peers(world, PEER_SAME_WORLD, [&effect](ENetPeer &p) { send_data(p, effect); });
        }

    auto list = [](const std::vector<::slot> &offer)
    {
        std::vector<std::string> names{};
        for (const ::slot &slot : offer) names.emplace_back(std::format("{} {}", slot.count, id_to_item(slot.id).raw_name));
        return names.empty() ? std::string{"nothing"} : join(names, ", ");
    };
    const std::string message = std::format("{} traded {} to {} for {}.",
        pTraders[0]->display_growid, list(give[0]), pTraders[1]->display_growid, list(give[1]));
    peers(world, PEER_SAME_WORLD, [&message](ENetPeer &p) { on::ConsoleMessage(&p, message); });

    for (std::size_t i = 0; i < 2; ++i)
    {
        send_varlist(traders[i], { "OnTextOverlay", "`2Trade complete!``" });
        send_action(*traders[i], "play_sfx", "file|audio/keypad_hit.wav\ndelayMS|0");
        pTraders[i]->trade = {};
    }
}

void trade_leave(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->netid == 0) return;

    const std::string message = std::format("`6[```4Trade canceled: {}`` left the world`4!```6]``", pPeer->display_growid);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&pPeer, &message](ENetPeer &peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer.data);
        if (pOthers->user_id != pPeer->user_id && pOthers->trade.netid == pPeer->netid) // @note our partner, or anyone waiting on us
            trade_end(peer, message);
    });
    pPeer->trade = {};
}
