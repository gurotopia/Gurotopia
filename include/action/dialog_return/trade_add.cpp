#include "pch.hpp"
#include "tools/trade.hpp"
#include "trade_add.hpp"

void trade_add(ENetEvent& event, const ::hPipe &hPipe)
{
    const short id = atoi(hPipe["itemID"].c_str());
    const int count = atoi(hPipe["count"].c_str());

    if (count < 0)
    {
        send_varlist(event.peer, { "OnTextOverlay", "Whatever scammer. Bet you liked the game more before this cool trade system!" });
        return;
    }
    if (std::string why{}; count == 0 || !tradeable(id, why)) return; // @note the dialog can be faked, so check again

    trade_offer(event, id, static_cast<short>(std::min(count, 200)));
}
