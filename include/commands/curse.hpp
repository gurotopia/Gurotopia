#pragma once

extern void curse_cmd(ENetEvent& event, const std::string_view text);
extern void uncurse_cmd(ENetEvent& event, const std::string_view text);
extern std::string curse_time_left(const ::peer &p);
extern std::string curse_wrench_line(const ::peer &p);
