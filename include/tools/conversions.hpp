#pragma once

enum : u_short/*fit in ::slot::count*/
{
    DIAMOND_LOCK_VALUE = 100,
    BLUE_GEM_LOCK_VALUE = 1000
};

void convert_lock(int amount, u_short& wl, u_short& dl, u_short& bgl, std::string &str);