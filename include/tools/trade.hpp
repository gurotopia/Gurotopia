#pragma once

/* @return the peer `pPeer` asked to trade with, nullptr if they left the world */
extern ENetPeer *trade_target(const ::peer &pPeer);

/* @return the peer we are trading with. nullptr unless both peers asked to trade with each other */
extern ENetPeer *trade_partner(const ::peer &pPeer);

/* @brief ask `target` to trade. opens the trade window for event.peer */
extern void trade_request(ENetEvent &event, ENetPeer &target);

/* @return false if the item can't be traded, `why` holds the reason */
extern bool tradeable(short id, std::string &why);

/* @brief set how many of an item event.peer offers, 0 removes it */
extern void trade_offer(ENetEvent &event, short id, short count);

/* @brief show `owner`'s offer in the trade window of `to` */
extern void trade_status(ENetPeer &to, const ::peer &owner, const std::string &flags);

/* @brief redraw the trade window(s). the deal changed, so both "accept" ticks reset */
extern void trade_update(ENetPeer &peer, bool deal_changed);

/* @brief close the trade window of `peer` and forget their offer */
extern void trade_end(ENetPeer &peer, const std::string &message = "");

/* @brief swap both offers. items are checked again, so nothing is lost or duplicated */
extern void trade_execute(ENetPeer &peer, ENetPeer &other);

/* @brief cancel every trade event.peer is part of. call this before they leave the world */
extern void trade_leave(ENetEvent &event);
