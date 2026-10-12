#include "pch.hpp"
#include "tools/trade.hpp"
#include "rem_trade.hpp"

void action::rem_trade(ENetEvent& event, const std::string& header)
{
    const short id = atoi(::hPipe{ header }["itemID"].c_str());

    trade_offer(event, id, 0);
}
