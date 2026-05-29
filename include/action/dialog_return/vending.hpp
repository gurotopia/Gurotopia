#pragma once

extern void vending(ENetEvent& event, const std::vector<std::string> &&pipes);
extern void show_vending_dialog(ENetEvent& event, ::world& world, ::peer& peer, const ::pos& pos);
