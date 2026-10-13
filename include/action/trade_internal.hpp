#pragma once

#include <charconv>

namespace trade
{
    using phase = ::peer::trade_state::phase;
    using connections = std::array<ENetPeer*, 2>;
    constexpr short kWorldKey = 1424;
    constexpr std::size_t kTradeSlots = 4;

    template<typename T>
    bool parse_number(const std::string &text, T &value)
    {
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    }

    ::peer &of(ENetPeer &connection);
    bool valid_event(const ENetEvent &event);
    bool valid_item(short id);
    bool tradable_item(short id);
    bool may_edit(const ::peer &player);
    short held(const ::peer &player, short id);
    int used_slots(const std::vector<::slot> &inventory);
    bool gives_key(const ::peer &player);
    ::world *current_world(const ::peer &player);
    ::block *world_lock(::world &world, ::pos &at);
    bool can_offer_key(const ::peer &player, short id);
    ENetPeer *partner_of(const ::peer &player);
    bool together(const ::peer &first, const ::peer &second);
    void status(ENetPeer &to, const ::peer &whose, bool accepted, bool unticked = false);
    void end_window(ENetPeer &connection);
    void talk_bubble(ENetPeer &connection, const std::string &text);
    void over_head(ENetPeer &connection, const std::string &text);
    void joined(ENetPeer &joiner, ENetPeer &waiting);
    void changed(ENetPeer &connection, const std::string &line);
    void confirm(ENetPeer &to, const ::peer &self, const ::peer &other, ::world &world);
    void quantity_dialog(ENetPeer &to, short id, int count);
    void price_dialog(ENetPeer &to);
    void announce_request(ENetPeer &from, ENetPeer &to);
    void open_confirmation(ENetPeer &acceptor, ENetPeer &other);
    void send_accept(ENetPeer &sender, ENetPeer &other);
    void notify_waiting(ENetPeer &first, ENetPeer &other);
    bool wait_for_seller(ENetPeer &connection, const ::peer &partner);
    void cancel_confirm(ENetPeer &connection, ENetPeer *other);
    void close_window(ENetPeer &connection);
    void offer(ENetPeer &connection, short id, int count);
    std::string items_text(const std::vector<::slot> &offer);
    void sell_world(::world &world, ::peer &seller, ::peer &buyer);
    void execute(ENetPeer &last, ENetPeer &first);
}
