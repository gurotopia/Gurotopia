#pragma once

extern bool item_use(ENetEvent &event, ::world &world, const ::item &item, ::gamePacket &gamePacket);
extern void useitems_tick(std::time_t now);

/* read by SetClothing */
extern int   item_effect_state(const ::peer &p);
extern u_int item_skin(const ::peer &p);
extern ::pos item_speed(const ::peer &p);
extern std::string item_effects_wrench(const ::peer &p);
extern int   double_jump_state(const ::peer &p);
extern std::string mods_wrench(const ::peer &p);
