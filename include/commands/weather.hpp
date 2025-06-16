#pragma once
#ifndef WEATHER_HPP
#define WEATHER_HPP

    extern int get_weather_id(unsigned item_id);

    extern void weather(ENetEvent& event, const std::string_view text);

#endif