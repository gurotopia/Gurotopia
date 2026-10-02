#pragma once

extern void birth_certificate_return(ENetEvent &event, const ::hPipe &hPipe);
extern void change_address_return(ENetEvent &event, const ::hPipe &hPipe);
extern void tile_cmd(ENetEvent& event, const std::string_view text);
extern void dumpitems_cmd(ENetEvent& event, const std::string_view text);
extern void deleteallworlds_cmd(ENetEvent& event, const std::string_view text);
extern void delete_all_worlds_return(ENetEvent &event, const ::hPipe &hPipe);
extern void fillworld_cmd(ENetEvent& event, const std::string_view text);
extern void fill_world_return(ENetEvent &event, const ::hPipe &hPipe);
extern void cleardrops_cmd(ENetEvent& event, const std::string_view text);
