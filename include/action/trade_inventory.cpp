#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "trade_internal.hpp"

namespace trade
{
    bool valid_quantity(const ::slot &item)
    {
        return item.count > 0 && item.count <= 200;
    }

    bool valid_key_offer(const ::peer &player, const ::slot &item, ::world &world)
    {
        if (item.id != kWorldKey) return true;
        ::pos tile{};
        return item.count == 1 && world.owner == player.user_id && world_lock(world, tile);
    }

    bool remove_offer(std::vector<::slot> &inventory, const ::slot &item,
                      const ::peer &player, ::world &world)
    {
        if (!valid_quantity(item)) return false;
        if (!tradable_item(item.id)) return false;
        auto slot = std::ranges::find(inventory, item.id, &::slot::id);
        if (slot == inventory.end()) return false;
        if (slot->count < item.count) return false;
        if (!valid_key_offer(player, item, world)) return false;
        slot->count -= item.count;
        return true;
    }

    bool receive_offer(std::vector<::slot> &inventory, const ::slot &item)
    {
        if (item.id == kWorldKey) return true;
        if (!valid_quantity(item)) return false;
        auto slot = std::ranges::find(inventory, item.id, &::slot::id);
        if (slot == inventory.end())
        {
            inventory.push_back(item);
            return true;
        }
        if (slot->count + item.count > 200) return false;
        slot->count += item.count;
        return true;
    }

    /* @note outgoing items can free slots used by the incoming offer. */
    bool final_inventory(const ::peer &player, const std::vector<::slot> &incoming,
                         ::world &world, std::vector<::slot> &after)
    {
        after = player.slots;
        for (const ::slot &item : player.trade.offer)
            if (!remove_offer(after, item, player, world)) return false;
        for (const ::slot &item : incoming)
            if (!receive_offer(after, item)) return false;
        return used_slots(after) <= player.slot_size;
    }

    void unequip_offer(::peer &player)
    {
        for (const ::slot &item : player.trade.offer)
        {
            if (held(player, item.id) != item.count) continue;
            const auto kind = id_to_item(item.id).cloth_type;
            if (kind == clothing::NONE) continue;
            if (player.clothing[kind] == item.id) player.clothing[kind] = 0;
        }
    }

    void commit_inventory(::peer &player, std::vector<::slot> &inventory)
    {
        unequip_offer(player);
        player.slots = std::move(inventory);
        player.update_effects();
    }

    // The backpack is already updated; this only sends its client delta.
    void inventory_delta(ENetPeer &connection, const ::slot &change)
    {
        ::gamePacket packet{.id = change.id};
        packet.type = change.count < 0 ? (-change.count << 16) | 0x000d : (change.count << 24) | 0x000d;
        send_data(connection, compress_state(packet));
    }

    void send_item(const connections &traders, ENetPeer &giver, ENetPeer &receiver,
                   const ::slot &item, int delay)
    {
        const ::peer &from = of(giver), &to = of(receiver);
        const ::blob flight = compress_state(::gamePacket{
            .type = 0x03000013, .netid = to.netid, .uid = from.netid,
            .id = delay, .punch = ::pos{item.id, item.id}
        });
        for (ENetPeer *connection : traders)
        {
            send_data(*connection, flight);
            send_varlist(connection, { "OnPlayPositioned", "audio/change_clothes.wav" }, from.netid, delay);
        }
        inventory_delta(giver, ::slot{item.id, static_cast<short>(-item.count)});
        if (item.id != kWorldKey) inventory_delta(receiver, item);
    }

    int send_offer(const connections &traders, ENetPeer &giver, ENetPeer &receiver,
                   ::world &world, int delay)
    {
        ::peer &from = of(giver), &to = of(receiver);
        const std::string name = from.display_growid; // @note before a sale changes the owner's colour
        const std::vector<::slot> offer = from.trade.offer;
        for (std::size_t i = 0; i < offer.size(); ++i)
            send_item(traders, giver, receiver, offer[i], delay + 300 + 350 * static_cast<int>(i));
        if (gives_key(from)) sell_world(world, from, to);
        delay += 300 + 350 * static_cast<int>(offer.size());
        const std::string line = std::format("`1{} traded {} to {}.``", name, items_text(offer), to.growid);
        on::ConsoleMessage(&giver, line, delay);
        on::ConsoleMessage(&receiver, line, delay);
        return delay;
    }

    void finish_trade(const connections &traders, int netid, int delay)
    {
        for (ENetPeer *connection : traders)
            send_varlist(connection, { "OnPlayPositioned", "audio/keypad_hit.wav" }, netid, delay + 300);
        for (ENetPeer *connection : traders) end_window(*connection);
    }

    void failed_trade(const connections &traders)
    {
        for (ENetPeer *connection : traders)
        {
            send_varlist(connection, { "OnTextOverlay", "`4The trade could not be completed.``" });
            end_window(*connection);
        }
    }

    /* @note effects start with the last confirmer, 350 ms apart. */
    void execute(ENetPeer &last, ENetPeer &first)
    {
        ::peer &a = of(last), &b = of(first);
        ::world *world = current_world(a);
        if (!world) return;
        const connections traders{&last, &first};
        std::vector<::slot> final_a, final_b;
        if (!final_inventory(a, b.trade.offer, *world, final_a) || !final_inventory(b, a.trade.offer, *world, final_b))
        {
            failed_trade(traders);
            return;
        }
        // Both backpacks must be checked before either one changes.
        commit_inventory(a, final_a);
        commit_inventory(b, final_b);
        std::printf("Trade in %s: %s gave %s; %s gave %s\n", world->name.c_str(), a.growid.c_str(),
                    items_text(a.trade.offer).c_str(), b.growid.c_str(), items_text(b.trade.offer).c_str());
        int delay = send_offer(traders, last, first, *world, 0);
        delay = send_offer(traders, first, last, *world, delay);
        finish_trade(traders, a.netid, delay);
    }
}
