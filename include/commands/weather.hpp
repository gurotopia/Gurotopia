#pragma once
#ifndef WEATHER_HPP
#define WEATHER_HPP

    extern int get_weather_id(u_int item_id);
    extern void refresh_world_weather_cache(::world& w);

    extern void weather(ENetEvent& event, const std::string_view text);

#endif
