#include "ransuu.hpp"

int ransuu::operator[](Range _range) {
    std::uniform_int_distribution<int> dist(_range.min, _range.max);
    return dist(engine);
}

float ransuu::shosu(Range _range, float right = 0.1f) {
    return static_cast<float>((*this)[_range]) * right;
}
