#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"
#include "worldlock_storage.hpp"
#include <algorithm>
#include <charconv>
#include <limits>

namespace
{
    struct denomination { short id; int value; const char *name; };
    constexpr denomination locks[3]{
        {7188, 10000, "Blue Gem Lock"}, {1796, 100, "Diamond Lock"}, {242, 1, "World Lock"}
    };
    struct operation { const char *type; int sign; const char *verb; };
    constexpr operation operations[2]{
        {"1", -1, "deposited"}, {"-1", 1, "collected"}
    };

    void response(ENetEvent &event)
    {
        const ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        const u_int count = pPeer->wl_storage;
        std::string data{"\x95\x90", 2}; // @note msgpack [[], balance, 0, 0, 0]
        struct encoding { u_int maximum; u_char prefix; int bytes; };
        constexpr encoding formats[]{
            {0x7f, 0, 1}, {0xff, 0xcc, 1}, {0xffff, 0xcd, 2}, {0xffffffffu, 0xce, 4}
        };
        const auto format = std::ranges::find_if(formats, [count](const encoding &entry) { return count <= entry.maximum; });
        if (format->prefix != 0) data.push_back(static_cast<char>(format->prefix));
        for (int i = format->bytes - 1; i >= 0; --i) data.push_back(static_cast<char>(count >> (i * 8)));
        data.append(3, '\0');
        send_varlist(event.peer, { "OnWorldLockStorageResponse", data });
    }

    bool deposit(int amount, const int (&held)[3], int (&changes)[3])
    {
        int spare = -1;
        for (int i = 0; i < 3; ++i)
        {
            changes[i] = -std::min(held[i], amount / locks[i].value);
            amount += changes[i] * locks[i].value;
            if (held[i] + changes[i] > 0) spare = i;
        }
        if (amount > 0)
        {
            if (spare < 0) return false;
            --changes[spare];
            amount -= locks[spare].value;
        }
        int change = -amount;
        for (int i = 0; i < 3; ++i)
        {
            changes[i] += change / locks[i].value;
            change %= locks[i].value;
        }
        return true;
    }

    void withdraw(int amount, const int (&held)[3], int (&changes)[3], int free)
    {
        for (int i = 0; i < 3; ++i)
        {
            const int count = std::min(amount / locks[i].value, 200);
            amount -= count * locks[i].value;
            if (count == 0) continue;
            if (held[i] == 0 && free <= 0) continue;
            changes[i] = std::min(count, 200 - held[i]);
            if (held[i] == 0) --free;
        }
    }

    bool fits(const int (&held)[3], const int (&changes)[3], int free)
    {
        for (int i = 0; i < 3; ++i)
        {
            const int count = held[i] + changes[i];
            if (count < 0 || count > 200) return false;
            free += (held[i] > 0) - (count > 0);
        }
        return free >= 0;
    }

    void apply(ENetEvent &event, const int (&changes)[3], const operation &op)
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        int value{};
        std::vector<std::string> parts;
        for (int i = 0; i < 3; ++i)
        {
            value += changes[i] * locks[i].value;
            if (changes[i] != 0) modify_item_inventory(event, {locks[i].id, static_cast<short>(changes[i])});
            const int count = changes[i] * op.sign;
            if (count <= 0) continue;
            parts.push_back(std::format("{} `2{}s``", count, locks[i].name));
        }
        if (value == 0) return;
        std::erase_if(pPeer->slots, [](const ::slot &slot) { return slot.count == 0; });
        pPeer->wl_storage -= value;
        pPeer->mysql_update_all();
        const std::string message = std::format("You {} {}.", op.verb, join(parts, " and "));
        send_varlist(event.peer, { "OnTalkBubble", static_cast<u_int>(pPeer->netid), message, 0u, 1u });
        on::ConsoleMessage(event.peer, message);
        response(event);
    }

    void transfer(ENetEvent &event, int amount, const operation &op)
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        if (op.sign < 0 && amount > std::numeric_limits<int>::max() - pPeer->wl_storage) return;
        int held[3]{}, changes[3]{};
        for (int i = 0; i < 3; ++i)
        {
            const auto it = std::ranges::find(pPeer->slots, locks[i].id, &::slot::id);
            if (it != pPeer->slots.end()) held[i] = it->count;
        }
        const int free = pPeer->slot_size - std::ranges::count_if(pPeer->slots, [](const ::slot &slot) { return slot.count > 0; });
        if (op.sign < 0)
        {
            if (!deposit(amount, held, changes)) return;
        }
        else withdraw(std::min(amount, pPeer->wl_storage), held, changes, free);
        if (!fits(held, changes, free)) return;
        apply(event, changes, op);
    }
}

void action::worldlock_storage(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->user_id <= 0 || pPeer->wl_storage < 0) return;
    const ::hPipe hPipe{header};
    if (hPipe["action"] == "worldlock_storage_getpage")
    {
        response(event);
        return;
    }
    const std::string input = hPipe["amount"];
    int amount{};
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), amount);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || amount <= 0) return;
    const auto op = std::ranges::find_if(operations, [&hPipe](const operation &entry) { return hPipe["type"] == entry.type; });
    if (op != std::end(operations)) transfer(event, amount, *op);
}
