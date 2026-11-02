#include "pch.hpp"

#include "conversions.hpp"

void convert_lock(int amount, u_short& wl, u_short& dl, u_short& bgl, std::string &str)
{
    bgl = std::min<int>(amount / BLUE_GEM_LOCK_VALUE, bgl);
    amount -= bgl * BLUE_GEM_LOCK_VALUE;

    dl = std::min<int>(amount / DIAMOND_LOCK_VALUE, dl);
    amount -= dl * DIAMOND_LOCK_VALUE;

    wl = std::min<int>(amount, wl);
    amount -= wl;

    if (bgl > 0)
        str = std::format("{} `2Blue Gem Lock`` and {} `2Diamond Lock`` and {} `2World Lock``", bgl, dl, wl);
    else if (dl > 0)
        str = std::format("{} `2Diamond Lock`` and {} `2World Lock``", dl, wl);
    else if (wl > 0)
        str = std::format("{} `2World Lock``", wl);
}