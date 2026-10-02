#pragma once

/* a player in this world by exact name, or by the first letters (3 or more, must be unique) */
extern ENetPeer *find_in_world(const std::string &world, const std::string &name, bool &ambiguous);

/* pulls a player in the same world to where you stand (world owners, mods and devs) */
extern bool pull_player(ENetEvent &event, ENetPeer &target);
extern void pull_cmd(ENetEvent &event, const std::string_view text);
