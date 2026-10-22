#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "trade_internal.hpp"

namespace trade
{
    /* @note Accept changes accepted, not locked. Offer resets also need reset_locks. */
    void status(ENetPeer &to, const ::peer &whose, bool accepted, bool unticked)
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

    void talk_bubble(ENetPeer &peer, const std::string &text)
    {
        send_varlist(&peer, { "OnTalkBubble", static_cast<u_int>(of(peer).netid), text, 0u, 0u });
    }

    void over_head(ENetPeer &peer, const std::string &text)
    {
        talk_bubble(peer, text);
        on::ConsoleMessage(&peer, text);
    }

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

    void quantity_dialog(ENetPeer &to, short id, int count)
    {
        const ::item &item = id_to_item(id);
        send_varlist(&to, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`2Trade`` `w{}``|left|{}|\n"
                "add_textbox|`2Trade how many?``|left|\n"
                "add_text_input|count||{}|5|\n"
                "embed_data|itemID|{}\n"
                "end_dialog|trade_item|Cancel|OK|\n",
                item.raw_name, id, count, id)
        });
    }

    void price_dialog(ENetPeer &to)
    {
        send_varlist(&to, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wTrade-Scan``|left|13816|\n"
            "add_spacer|small|\n"
            "add_textbox|No items to check the pricing for!|left|\n"
            "end_dialog|p2p_trade_price_check_items_ui|Back||\n"
            "add_quick_exit|\n"
        });
    }

    void announce_request(ENetPeer &from, ENetPeer &to)
    {
        const ::peer &player = of(from), &target = of(to);
        send_varlist(&from, { "OnStartTrade", target.growid, target.netid });
        if (target.trade.partner_netid == player.netid) return;
        on::ConsoleMessage(&to, std::format(
            "`#TRADE ALERT:`` `w{0}`` wants to trade with you!  To start, use the `wWrench`` on that person's wrench icon, or type `w/trade {0}",
            player.display_growid));
        send_action(to, "play_sfx", "file|audio/cash_register.wav\ndelayMS|0");
    }

    void open_confirmation(ENetPeer &acceptor, ENetPeer &other)
    {
        ::peer &first = of(acceptor), &second = of(other);
        ::world *world = current_world(first);
        if (!world) return;
        first.trade.stage = second.trade.stage = phase::confirming;
        first.trade.confirmed = second.trade.confirmed = false;
        for (ENetPeer *to : {&acceptor, &other})
        {
            const ::peer &self = of(*to);
            const ::peer &them = to == &acceptor ? second : first;
            status(*to, first, true);
            send_varlist(to, { "OnForceTradeEnd" });
            confirm(*to, self, them, *world);
        }
    }

    /* @note reset the other side before sending the changed Accept state. */
    void send_accept(ENetPeer &sender, ENetPeer &other)
    {
        const ::peer &first = of(sender), &second = of(other);
        status(sender, first, first.trade.accepted, !first.trade.accepted);
        status(other, second, true);
        status(other, second, false);
        status(other, first, first.trade.accepted, !first.trade.accepted);
    }

    void notify_waiting(ENetPeer &first, ENetPeer &other)
    {
        of(first).trade.confirmed = true;
        over_head(first, "`6[``Trade accepted, waiting for other player to accept`6]``");
        over_head(other, "`6[``Trade accepted by other player, waiting for you`6]``");
    }

    /* @note the seller must confirm before the buyer. */
    bool wait_for_seller(ENetPeer &connection, const ::peer &partner)
    {
        if (!gives_key(partner) || partner.trade.confirmed) return false;
        ::peer &player = of(connection);
        ::world *world = current_world(player);
        if (!world) return true;
        send_varlist(&connection, { "OnTextOverlay", "`6[``you must wait for the other player to accept first`6]``" });
        send_varlist(&connection, { "OnForceTradeEnd" });
        confirm(connection, player, partner, *world);
        return true;
    }
}
