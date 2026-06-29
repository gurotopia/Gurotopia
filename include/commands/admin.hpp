#pragma once

void give(ENetEvent& event, const std::string_view text);
void setgems(ENetEvent& event, const std::string_view text);
void ban(ENetEvent& event, const std::string_view text);
void unban(ENetEvent& event, const std::string_view text);
void mute(ENetEvent& event, const std::string_view text);
void unmute(ENetEvent& event, const std::string_view text);
void kick(ENetEvent& event, const std::string_view text);
void setrole(ENetEvent& event, const std::string_view text);