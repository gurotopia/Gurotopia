#pragma once

/* the staff part of the wrench menu when a mod/dev wrenches a player */
extern std::string staff_wrench_rows(const ::peer &viewer, const ::peer &target, ENetPeer &target_peer);
extern bool staff_popup(ENetEvent &event, const ::hPipe &hPipe); // @return true if it was a staff button
