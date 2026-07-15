#pragma once

enum holiday : u_char
{
    H_NONE,
    H_VALENTINES,
    H_PATRICKS,
    H_SUMMERFEST = 6,
    H_G4G = 18
};
extern u_char holiday;

extern std::tm localtime();

extern void check_for_holiday();

extern std::string game_theme_string();

extern std::pair<std::string, std::string> holiday_greeting();

extern std::string holiday_banner();
