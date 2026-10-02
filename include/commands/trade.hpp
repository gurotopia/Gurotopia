#pragma once

/* native Growtopia trading: wrench > Trade or /trade {player} opens the real trade window */
extern void trade_request(ENetEvent &event, ENetPeer &target);
extern void trade_cmd(ENetEvent &event, const std::string_view text);
extern void trade_end_for(const ::peer &p, const std::string &why); // @note leaving a world or the game cancels the trade

/* packets the client sends while the trade window is open */
namespace action
{
    extern void trade_started(ENetEvent &event, const std::string &header);
    extern void mod_trade(ENetEvent &event, const std::string &header);
    extern void rem_trade(ENetEvent &event, const std::string &header);
    extern void trade_accept(ENetEvent &event, const std::string &header);
    extern void trade_cancel(ENetEvent &event, const std::string &header);
}
extern void trade_return(ENetEvent &event, const ::hPipe &hPipe); // @note dialogs "trade_add" and "trade_confirm"
