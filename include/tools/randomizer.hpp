#ifndef RANDOMIZER_HPP
#define RANDOMIZER_HPP

    #include <random>

    extern thread_local std::mt19937_64 engine;

    extern int randomizer(int min, int max);
    extern float randomizer(float min, float max);

#endif