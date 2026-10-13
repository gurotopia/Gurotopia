#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "trade_internal.hpp"

namespace trade
{
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

    bool can_offer_key(const ::peer &player, short id)
    {
        if (id != kWorldKey) return true;
        const ::world *world = current_world(player);
        return world && world->owner == player.user_id;
    }

    bool key_available(const ::peer &player, ::world &world)
    {
        if (world.owner != player.user_id) return false;
        if (held(player, kWorldKey) > 0) return false;
        ::pos tile{};
        return world_lock(world, tile) != nullptr;
    }

    enum class key_check { ready, coowners, loose_items, invalid_block, untradeable_block, full_backpack };

    key_check check_block(const ::block &block)
    {
        for (short id : {block.fg, block.bg})
        {
            if (id < 0 || id >= static_cast<int>(items.size())) return key_check::invalid_block;
            if (id && (id_to_item(id).cat & CAT_UNTRADEABLE)) return key_check::untradeable_block;
        }
        return key_check::ready;
    }

    key_check check_blocks(const ::world &world)
    {
        for (const ::block &block : world.blocks)
        {
            const key_check result = check_block(block);
            if (result != key_check::ready) return result;
        }
        return key_check::ready;
    }

    key_check check_key(const ::peer &player, const ::world &world)
    {
        if (std::ranges::any_of(world.access, [](int uid) { return uid != 0; })) return key_check::coowners;
        if (!world.objects.empty()) return key_check::loose_items;
        const key_check blocks = check_blocks(world);
        if (blocks != key_check::ready) return blocks;
        if (used_slots(player.slots) >= player.slot_size) return key_check::full_backpack;
        return key_check::ready;
    }

    const char *key_refusal(key_check result)
    {
        switch (result)
        {
            case key_check::coowners:
                return "You'll first need to `4remove all co-owners`` from your `5World Lock`` to get a `#World Key`` to trade this world.";
            case key_check::loose_items:
                return "`4Oops!... Can't trade a world with floating items that are unblocked or in treasure chests!``";
            case key_check::untradeable_block:
                return "`4Oops!... Can't trade a world with Untradeable blocks in it!``";
            case key_check::full_backpack:
                return "You need more backpack room first!";
            default:
                return nullptr;
        }
    }

}

void trade_world_key(ENetEvent &event)
{
    if (!trade::valid_event(event)) return;
    ::peer &player = trade::of(*event.peer);
    ::world *world = trade::current_world(player);
    if (!world) return;
    if (!trade::key_available(player, *world)) return;
    const trade::key_check result = trade::check_key(player, *world);
    if (result != trade::key_check::ready)
    {
        const char *text = trade::key_refusal(result);
        if (text) trade::talk_bubble(*event.peer, text);
        return;
    }
    trade::talk_bubble(*event.peer, "You got a `#World Key``! You can now trade this world to other players.");
    modify_item_inventory(event, ::slot{trade::kWorldKey, 1});
    send_action(*event.peer, "play_sfx", "file|audio/use_lock.wav\ndelayMS|0");
}
