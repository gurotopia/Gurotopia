#pragma once
#ifndef CMD_PUNCH_HPP
#define CMD_PUNCH_HPP

    extern u_char get_punch_id(u_int item_id);

    extern void punch(ENetEvent& event, const std::string_view text);

#endif