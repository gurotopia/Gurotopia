#pragma once

extern void shutdown_cmd(ENetEvent &event, const std::string_view text);
extern void shutdown_tick(std::time_t now); // @note once a second: announces the countdown and shuts down at 0
