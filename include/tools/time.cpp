#include "pch.hpp"

#include "time.hpp"

std::time_t ticks()
{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec;
}
