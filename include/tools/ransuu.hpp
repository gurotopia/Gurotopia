// repo:  https://github.com/leeendl/ransuu

#include <time.h>

#define l_max 2147483647l
#define l_min -l_max+1

#if __cplusplus == 201103L
    #include <initializer_list>
    template<typename T> 
        using range = std::initializer_list<T>;
#elif __cplusplus > 201103L
template<typename T>
struct range
{
    T front{}, back{};
};
#endif

class ransuu
{
    struct timespec ts;
public:
    template<typename T = long>
        T operator[](range<T> _range) 
        {
            clock_gettime(CLOCK_MONOTONIC, &this->ts);
            long val = ts.tv_nsec 
                ^ (ts.tv_nsec >> 5)
                ^ (ts.tv_sec << 3);
                
            (val < l_min) ? val = l_min : 
            (l_max < val) ? val = l_max : val;
            #if __cplusplus == 201103L
                return static_cast<T>( val % (*(_range.begin() + 1) - *_range.begin() + 1) + *_range.begin() );
            #elif __cplusplus > 201103L
                return static_cast<T>( val % (++_range.back - _range.front) + _range.front );
            #endif
        };
    template<typename T = long>
        float shosu(range<T> _range, float right = 0.1f)
        {
            return static_cast<float>((*this)[_range]) * right;
        }
};

#undef l_max
#undef l_min