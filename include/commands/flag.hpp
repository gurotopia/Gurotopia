#pragma once

extern std::string flag_of(const ::peer &p); // @note the flag beside a name: chosen with /flag, else the game's own country
extern void flag_cmd(ENetEvent &event, const std::string_view text);