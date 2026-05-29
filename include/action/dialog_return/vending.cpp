#include "pch.hpp"

#include "tools/create_dialog.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "on/SetBux.hpp"

#include "vending.hpp"

namespace
{
    constexpr short WORLD_LOCK_ID = 242;
    constexpr short DIAMOND_LOCK_ID = 1796;
    constexpr short BLUE_GEM_LOCK_ID = 7188;
    constexpr int DIGIVEND_UPGRADE_COST = 4000;

    ::vending_machine* find_vend(::world& world, const ::pos& pos)
    {
        auto it = std::ranges::find(world.vendings, pos, &::vending_machine::pos);
        return (it == world.vendings.end()) ? nullptr : &*it;
    }

    ::vending_machine& ensure_vend(::world& world, const ::pos& pos)
    {
        if (auto* vend = find_vend(world, pos)) return *vend;
        world.vendings.emplace_back(pos);
        return world.vendings.back();
    }

    bool is_owner_or_admin(const ::world& world, const ::peer& peer)
    {
        return peer.user_id == world.owner || std::ranges::contains(world.admin, peer.user_id) || peer.role == DEVELOPER;
    }

    int get_count(const ::peer& peer, short id)
    {
        auto it = std::ranges::find(peer.slots, id, &::slot::id);
        return (it == peer.slots.end()) ? 0 : it->count;
    }

    int total_world_locks(const ::peer& peer)
    {
        return get_count(peer, WORLD_LOCK_ID) + get_count(peer, DIAMOND_LOCK_ID) * 100 + get_count(peer, BLUE_GEM_LOCK_ID) * 10000;
    }

    void set_slot_count(ENetEvent& event, short id, int new_count)
    {
        ::peer* peer = static_cast<::peer*>(event.peer->data);
        auto it = std::ranges::find(peer->slots, id, &::slot::id);
        const int current = (it == peer->slots.end()) ? 0 : it->count;
        const int delta = new_count - current;
        if (delta != 0) modify_item_inventory(event, {id, static_cast<short>(delta)});
    }

    void set_total_world_locks(ENetEvent& event, int total)
    {
        total = std::max(0, total);
        const int bgl = total / 10000;
        total %= 10000;
        const int dl = total / 100;
        total %= 100;
        const int wl = total;

        set_slot_count(event, BLUE_GEM_LOCK_ID, bgl);
        set_slot_count(event, DIAMOND_LOCK_ID, dl);
        set_slot_count(event, WORLD_LOCK_ID, wl);
    }

    bool can_hold_more(const ::peer& peer, short id, int add)
    {
        auto it = std::ranges::find(peer.slots, id, &::slot::id);
        const int current = (it == peer.slots.end()) ? 0 : it->count;
        return current + add <= 200;
    }

    int read_int(const std::vector<std::string>& pipes, std::string_view key, int fallback = 0)
    {
        for (std::size_t i = 0; i + 1 < pipes.size(); ++i)
            if (pipes[i] == key)
            {
                try { return std::stoi(pipes[i + 1]); }
                catch (...) { return fallback; }
            }
        return fallback;
    }

    bool has_token(const std::vector<std::string>& pipes, std::string_view key)
    {
        return std::ranges::find(pipes, key) != pipes.end();
    }

    std::string vend_item_name(short id)
    {
        auto item = std::ranges::find(items, id, &::item::id);
        return (item == items.end()) ? std::string{"Nothing"} : item->raw_name;
    }

    std::string vend_price_text(const ::vending_machine& vend)
    {
        if (vend.item_id == 0 || vend.stock <= 0) return "Not configured";
        if (vend.per_item) return std::format("`2{} World Lock(s) per item``", vend.price);
        return std::format("`2{} item(s) per World Lock``", vend.price);
    }

    void refresh_vend_tile(ENetEvent& event, ::world& world, const ::pos& pos)
    {
        send_tile_update(event, {.id = world.blocks[cord(pos.x, pos.y)].fg, .punch = pos}, world.blocks[cord(pos.x, pos.y)], world);
    }

    void show_retrieve_stock_dialog(ENetEvent& event, const ::world& world, const ::pos& pos, const ::vending_machine& vend)
    {
        if (vend.item_id == 0 || vend.stock <= 0)
        {
            packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4There is no stock to take back.``"});
            return;
        }

        std::string dialog;
        dialog += "set_default_color|`o\n";
        dialog += std::format("add_label_with_icon|big|`wTake Stock Back``|left|{}|\n", world.blocks[cord(pos.x, pos.y)].fg);
        dialog += std::format("embed_data|tilex|{}\nembed_data|tiley|{}\n", pos.x_int(), pos.y_int());
        dialog += "embed_data|retrieve_stock|1\n";
        dialog += std::format("add_label_with_icon|small|`w{} ``(`2{} in stock``)|left|{}|\n", vend_item_name(vend.item_id), vend.stock, vend.item_id);
        dialog += std::format("add_text_input|takestockcount|How many do you want to take back?:|{}|5|\n", vend.stock);
        dialog += "end_dialog|vending|Back|Take|\n";
        packet::create(*event.peer, false, 0, {"OnDialogRequest", dialog.c_str()});
    }
}

void show_vending_dialog(ENetEvent& event, ::world& world, ::peer& peer, const ::pos& pos)
{
    auto& vend = ensure_vend(world, pos);
    const bool owner = is_owner_or_admin(world, peer);
    const auto tile = world.blocks[cord(pos.x, pos.y)].fg;

    std::string dialog;
    dialog += "set_default_color|`o\n";
    dialog += std::format("add_label_with_icon|big|`w{}``|left|{}|\n", vend.digivend ? "DigiVend" : "Vending Machine", tile);
    dialog += std::format("embed_data|tilex|{}\nembed_data|tiley|{}\n", pos.x_int(), pos.y_int());
    dialog += "add_spacer|small|\n";

    if (owner)
    {
        dialog += std::format("add_textbox|`wStock Item: ``{}``|left|\n", vend_item_name(vend.item_id));
        dialog += std::format("add_textbox|`wStock: ``{}``|left|\n", vend.stock);
        dialog += std::format("add_textbox|`wMode: ``{}``|left|\n", vend.per_item ? "WLs per item" : "Items per WL");
        dialog += std::format("add_textbox|`wPrice: ``{}``|left|\n", vend_price_text(vend));
        if (vend.earned_wls > 0)
            dialog += std::format("add_textbox|`wEarnings Ready: ``{} WL(s)``|left|\n", vend.earned_wls);

        dialog += "add_spacer|small|\n";
        if (vend.item_id == 0 || vend.stock <= 0)
            dialog += "add_item_picker|stockitem|`wSelect item to stock``|Choose an item to put in the machine!|\n";
        else
            dialog += std::format("add_label_with_icon|small|`wSelected Item: {}``|left|{}|\n", vend_item_name(vend.item_id), vend.item_id);

        dialog += std::format("add_text_input|setprice|Price:|{}|6|\n", std::max(1, vend.price));
        dialog += std::format("add_checkbox|chk_peritem|World Locks per Item|{}\n", vend.per_item ? "1" : "0");
        dialog += std::format("add_checkbox|chk_perlock|Items per World Lock|{}\n", vend.per_item ? "0" : "1");

        dialog += "add_spacer|small|\n";
        if (vend.item_id != 0)
            dialog += "add_button|placeall|Place All Items In Vend|noflags|0|0|\n";
        dialog += "add_button|collectstock|Take Stock Back|noflags|0|0|\n";
        if (vend.earned_wls > 0)
            dialog += "add_button|collectlocks|Collect World Locks|noflags|0|0|\n";
        if (!vend.digivend)
        {
            dialog += std::format("add_smalltext|Upgrade to a DigiVend Machine for `4{} Gems``.|left|\n", DIGIVEND_UPGRADE_COST);
            dialog += "add_button|upgradedigital|Upgrade to DigiVend|noflags|0|0|\n";
        }
        dialog += "add_spacer|small|\n";
        dialog += "end_dialog|vending|Close|Update|\n";
    }
    else
    {
        if (vend.item_id == 0 || vend.stock <= 0)
        {
            dialog += "add_textbox|This machine is empty.|left|\n";
            dialog += "end_dialog|vending|Close||\n";
        }
        else
        {
            dialog += std::format("add_label_with_icon|small|`w{}``|left|{}|\n", vend_item_name(vend.item_id), vend.item_id);
            dialog += std::format("add_textbox|`wIn Stock: ``{}``|left|\n", vend.stock);
            dialog += std::format("add_textbox|`wPrice: ``{}``|left|\n", vend_price_text(vend));
            if (vend.per_item)
                dialog += "add_text_input|buycount|How many items do you want?:|1|5|\n";
            else
                dialog += "add_text_input|buycount|How many World Locks do you want to spend?:|1|5|\n";
            dialog += "add_spacer|small|\n";
            dialog += "end_dialog|vending|Close|Buy|\n";
        }
    }

    packet::create(*event.peer, false, 0, {"OnDialogRequest", dialog.c_str()});
}

void vending(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer* peer = static_cast<::peer*>(event.peer->data);
    if (!peer) return;

    auto world = std::ranges::find(worlds, peer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    const ::pos pos{read_int(pipes, "tilex"), read_int(pipes, "tiley")};
    auto& vend = ensure_vend(*world, pos);
    const bool owner = is_owner_or_admin(*world, *peer);

    if (owner && read_int(pipes, "retrieve_stock", 0) != 0)
    {
        const int requested = std::max(1, read_int(pipes, "takestockcount", vend.stock));
        const int take_amount = std::min(requested, vend.stock);
        if (vend.item_id == 0 || vend.stock <= 0) { show_vending_dialog(event, *world, *peer, pos); return; }
        if (!can_hold_more(*peer, vend.item_id, take_amount))
        {
            packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4You don't have enough room for that much stock.``"});
            show_retrieve_stock_dialog(event, *world, pos, vend);
            return;
        }
        modify_item_inventory(event, {vend.item_id, static_cast<short>(take_amount)});
        vend.stock -= take_amount;
        if (vend.stock <= 0)
        {
            vend.stock = 0;
            vend.item_id = 0;
        }
        refresh_vend_tile(event, *world, pos);
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    if (has_token(pipes, "upgradedigital") && owner)
    {
        if (vend.digivend) { show_vending_dialog(event, *world, *peer, pos); return; }
        if (peer->gems < DIGIVEND_UPGRADE_COST)
        {
            const auto msg = std::format("`4You need {} Gems to upgrade this vending machine!``", DIGIVEND_UPGRADE_COST);
            packet::create(*event.peer, false, 0, {"OnConsoleMessage", msg.c_str()});
            show_vending_dialog(event, *world, *peer, pos);
            return;
        }
        peer->gems -= DIGIVEND_UPGRADE_COST;
        vend.digivend = true;
        on::SetBux(event);
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    if (has_token(pipes, "collectlocks") && owner)
    {
        if (vend.earned_wls <= 0) { show_vending_dialog(event, *world, *peer, pos); return; }
        if (total_world_locks(*peer) + vend.earned_wls > 20000)
        {
            packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4Collect some lock space first before taking the vending proceeds.``"});
            show_vending_dialog(event, *world, *peer, pos);
            return;
        }
        set_total_world_locks(event, total_world_locks(*peer) + vend.earned_wls);
        vend.earned_wls = 0;
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    if (has_token(pipes, "collectstock") && owner)
    {
        show_retrieve_stock_dialog(event, *world, pos, vend);
        return;
    }

    if (owner)
    {
        int selected_item = read_int(pipes, "stockitem", vend.item_id);
        int set_price = std::max(1, read_int(pipes, "setprice", std::max(1, vend.price)));
        const bool per_item = read_int(pipes, "chk_peritem", vend.per_item ? 1 : 0) != 0;
        const bool per_lock = read_int(pipes, "chk_perlock", vend.per_item ? 0 : 1) != 0;

        if (selected_item == 18 || selected_item == 32) selected_item = 0;

        if ((vend.item_id == 0 || vend.stock <= 0) && selected_item != 0)
            vend.item_id = static_cast<short>(selected_item);

        if (has_token(pipes, "placeall"))
        {
            if (vend.item_id == 0)
            {
                packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4Select an item first.``"});
                show_vending_dialog(event, *world, *peer, pos);
                return;
            }

            auto inv = std::ranges::find(peer->slots, vend.item_id, &::slot::id);
            if (inv == peer->slots.end() || inv->count <= 0)
            {
                packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4You don't have any more of that item in your inventory.``"});
                show_vending_dialog(event, *world, *peer, pos);
                return;
            }

            const int moved = inv->count;
            vend.stock += moved;
            modify_item_inventory(event, {vend.item_id, static_cast<short>(-moved)});
        }

        vend.per_item = per_lock ? false : true;
        vend.price = set_price;
        if (!vend.digivend) vend.price = std::min(vend.price, 200);
        refresh_vend_tile(event, *world, pos);
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    if (vend.item_id == 0 || vend.stock <= 0)
    {
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    int input = std::max(1, read_int(pipes, "buycount", 1));
    int item_amount = 0;
    int cost_wls = 0;
    if (vend.per_item)
    {
        item_amount = input;
        cost_wls = input * std::max(1, vend.price);
    }
    else
    {
        cost_wls = input;
        item_amount = input * std::max(1, vend.price);
    }

    item_amount = std::min(item_amount, vend.stock);
    if (item_amount <= 0) { show_vending_dialog(event, *world, *peer, pos); return; }
    if (!can_hold_more(*peer, vend.item_id, item_amount))
    {
        packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4You don't have enough room for that purchase.``"});
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }
    if (total_world_locks(*peer) < cost_wls)
    {
        packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`4You don't have enough World Locks.``"});
        show_vending_dialog(event, *world, *peer, pos);
        return;
    }

    const short bought_item_id = vend.item_id;
    const std::string bought_item_name = vend_item_name(bought_item_id);

    const int buyer_total = total_world_locks(*peer);
    set_total_world_locks(event, buyer_total - cost_wls);
    modify_item_inventory(event, {bought_item_id, static_cast<short>(item_amount)});
    vend.earned_wls += cost_wls;
    vend.stock -= item_amount;
    if (vend.stock <= 0)
    {
        vend.stock = 0;
        vend.item_id = 0;
    }

    const std::string buyer_msg = std::format("You bought `w{} {}`` for `2{} World Lock(s)``.", item_amount, bought_item_name, cost_wls);
    packet::create(*event.peer, false, 0, {"OnConsoleMessage", buyer_msg.c_str()});

    refresh_vend_tile(event, *world, pos);
    show_vending_dialog(event, *world, *peer, pos);
}
