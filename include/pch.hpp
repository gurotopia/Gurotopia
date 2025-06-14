#ifndef PCH_HPP
#define PCH_HPP

    #include <unordered_map>
    #include <vector>
    #include <algorithm>
    #include <ranges>
    #include <chrono>
    #include <thread>
    #include <fstream>
    #include <format>

    #include "mimalloc/mimalloc.h" // @note https://github.com/microsoft/mimalloc
    #include "nlohmann/json.hpp" // @note https://github.com/nlohmann/json

    #include "database/items.hpp"
    #include "database/peer.hpp"
    #include "database/world.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
    /* cause MSVC does not know 'zu' when the compiler(MSBuild) does... */
    #pragma warning(disable: 4455)
    constexpr std::size_t operator"" zu(std::size_t size) {
        return size;
    }
#endif

#endif