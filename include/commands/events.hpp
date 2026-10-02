#pragma once

extern void events_tick(std::time_t now);
extern void event_object_taken(ENetEvent &event, ::world &w, u_int uid, short id);
extern void event_cmd(ENetEvent& event, const std::string_view text);
extern bool event_hidden_object(::world &w);
extern bool event_party(::world &w);
