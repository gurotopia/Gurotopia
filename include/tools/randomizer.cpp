#include "randomizer.hpp"

#include <random>

static std::mt19937_64 engine{ std::random_device{}() };

int randomizer(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(engine);
}

float randomizer(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(engine);
}
