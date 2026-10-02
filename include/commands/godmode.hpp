#pragma once

extern const std::vector<u_char> &items_for(ENetEvent &event);
extern u_int items_hash_for(const ::peer &p);
extern void  god_items_changed(ENetEvent &event);
extern void  god_rejoin_tick(std::time_t now);
