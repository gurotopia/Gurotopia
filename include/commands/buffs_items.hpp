#pragma once
#include "buffs.hpp"

extern bool  buff_use(ENetEvent &event, const ::item &item);
extern void  buff_on_break(ENetEvent &event, ::world &world, const ::item &item, const ::gamePacket &gamePacket);
extern u_int buff_tree_head_start(const ::peer &p, const ::item &seed);
