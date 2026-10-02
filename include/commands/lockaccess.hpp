#pragma once

/* who a player is in a world: the one place that decides the colour of a name */
extern std::string display_name_for(const ::peer &p, const ::world &w);
extern void refresh_display_names(::world &w);
extern std::string chat_name(const ::peer &p); // @note the name as it looks where they stand right now (/msg) // @note recolours everyone in the world and tells everyone, no rejoin needed

/* World Lock access list (used by the wrench menu and /access) */
extern std::string access_name_of(int uid);
extern bool lock_access_add(ENetEvent &event, ::world &w, int uid);
extern bool lock_access_remove(ENetEvent &event, ::world &w, int uid);
extern std::string lock_access_rows(const ::world &w);
extern void lock_access_apply(ENetEvent &event, ::world &w, const ::hPipe &hPipe);
