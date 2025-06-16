#pragma once
#ifndef PCH_HPP
#define PCH_HPP

    #include "mimalloc/mimalloc-new-delete.h" // @note https://github.com/microsoft/mimalloc
    #include <unordered_map>
    #include <vector>
    #include <algorithm>
    #include <ranges>
    #include <chrono>
    #include <thread>
    #include <fstream>
    #include <format>
    #include "nlohmann/json.hpp" // @note https://github.com/nlohmann/json

    #include "database/items.hpp"
    #include "database/peer.hpp"
    #include "database/world.hpp"

#if defined(_MSC_VER)
    /* cause MSVC does not know 'zu' when the compiler(MSBuild) does... */
    #pragma warning(disable: 4455)
    constexpr std::size_t operator"" zu(std::size_t size) {
        return size;
    }
#endif

#endif