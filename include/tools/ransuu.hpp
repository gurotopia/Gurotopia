#pragma once

#include <random>

struct Range
{
    int min, max;
};

class ransuu
{
    std::mt19937 engine;

    public:
    ransuu() : engine(std::random_device{}()) {}

    int operator[](Range _range);

    float shosu(Range _range, float right);
};