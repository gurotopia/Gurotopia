#pragma once

extern bool is_jammer(u_short id);
extern bool world_jammer(const ::world &w, u_short id);
extern bool world_jammer(const std::string &world_name, u_short id);
extern bool jammer_punch(ENetEvent &event, ::world &w, ::gamePacket &gamePacket);
extern bool jammer_blocks(ENetEvent &event, const ::world &w, short item_id);
extern std::string jam_suffix(const std::string &world_name);
extern void jammer_wrench(ENetEvent &event, ::world &w, ::block &b, ::gamePacket &gamePacket);
extern void jammer_edit_return(ENetEvent &event, const ::hPipe &hPipe);
