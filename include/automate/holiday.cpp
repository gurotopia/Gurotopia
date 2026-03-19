#include "pch.hpp"

#include "holiday.hpp"

u_char holiday = H_NONE;

std::tm localtime()
{
    std::time_t now  = std::time(nullptr);

    return *std::localtime(&now);
}

void check_for_holiday()
{
    std::tm time = localtime();

    if (time.tm_mon == 1/*feb*/ && (time.tm_mday >= 13 && time.tm_mday <= 13+7)) holiday = H_VALENTINES;
    if (time.tm_mon == 2/*march*/ && (time.tm_mday >= 14 && time.tm_mday <= 14+7)) holiday = H_PATRICKS;
}

std::string game_theme_string()
{
    switch (holiday)
    {
        case H_VALENTINES: return "valentines-theme";
        case H_PATRICKS:   return "patricks-theme";
    }
    return "";
}

std::pair<std::string, std::string> holiday_greeting()
{
    switch (holiday)
    {
        case H_VALENTINES: return {"`5Valentine's Week!``", "`4Happy Valentine's Week!``"};
        case H_PATRICKS:   return {"`5St. Patrick's Week!``", "`2Happy St. Patrick's Day!``"};
    }
    return {"", ""};
}

std::string holiday_banner()
{
    switch (holiday)
    {
        case H_VALENTINES: return "interface/large/gui_valentine_banner.rttex";
    }
    return "interface/large/news_banner.rttex"; // @note default:
}