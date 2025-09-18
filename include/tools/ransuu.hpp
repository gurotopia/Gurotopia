#include <random>

template<typename T>
struct range
{
    T front{}, back{};
};

class ransuu
{
    std::mt19937 engine; // @todo change model(?)

    public:
    ransuu() : engine(std::random_device{}()) {}

    template<typename T = long>
    T operator[](range<T> _range) {
        std::uniform_int_distribution<T> dist(_range.front, _range.back);
        return dist(engine);
    }

    template<typename T = long>
    float shosu(range<T> _range, float right = 0.1f) {
        return static_cast<float>((*this)[_range]) * right;
    }
};