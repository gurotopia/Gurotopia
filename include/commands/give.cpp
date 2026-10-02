#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "give.hpp"

void give(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (text.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /give {itemID} {amount}``");
        return;
    }

    // split on the first space: "{id} {amount}"
    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    if (arg.empty()) { on::ConsoleMessage(event.peer, "`4Usage: /give {itemID} {amount}``"); return; }
    std::string id_str{}, amount_str{};

    if (const std::size_t space = arg.find(' '); space != std::string::npos)
    {
        id_str = arg.substr(0, space);
        amount_str = arg.substr(space + 1);
    }
    else
    {
        id_str = arg;
        amount_str = "200"; // default to a full stack
    }

    auto digits_only = [](std::string s)
    {
        std::erase_if(s, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
        return s;
    };

    id_str = digits_only(id_str);
    amount_str = digits_only(amount_str);

    if (id_str.empty() || amount_str.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /give {itemID} {amount}``");
        return;
    }

    signed id{}, amount{};
    try
    {
        id = std::stoi(id_str);
        amount = std::stoi(amount_str);
    }
    catch (...)
    {
        on::ConsoleMessage(event.peer, "`4That is not a valid number.``");
        return;
    }

    amount = std::clamp(amount, 1, 200);

    modify_item_inventory(event, ::slot(id, amount));
    on::ConsoleMessage(event.peer, std::format("`2Gave `w{}`` of item ID `w{}``.``", amount, id));
}
