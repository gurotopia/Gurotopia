#pragma once

extern void blast_create(ENetEvent &event, int id, std::string name);
extern void blast_treasure_break(ENetEvent &event, ::world &world, const ::item &item, const ::gamePacket &gamePacket);
extern bool blast_chest_punch(ENetEvent &event, ::world &world, ::block &block, const ::gamePacket &gamePacket);
