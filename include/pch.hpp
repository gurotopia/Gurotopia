#pragma once
#ifndef PCH_HPP
#define PCH_HPP

    #include "mimalloc/mimalloc-new-delete.h" // @note https://github.com/microsoft/mimalloc

    #include <cstring>
    #include <unordered_map>
    #include <vector>
    #include <algorithm>
    #include <ranges>
    #include <chrono>
    #include <thread>
    #include <fstream>
    #include <format>
    #include "sqlite/sqlite3.h" // @version SHA: a67c71224f5821547040b637aad7cddf4ef0778a (24/6/25)
    #include "enet/enet.h" // @version SHA: 657eaf97d9d335917c58484a4a4b5e03838ebd8e (13/11/24)
    #include "ssl/openssl/ssl.h" // @version SHA: 7bdc0d13d2b9ce1c1d0ec1f89dacc16e5d045314 (26/6/25)

    #include "database/items.hpp"
    #include "database/peer.hpp"
    #include "database/world.hpp"
    #include "packet/packet.hpp"

#if defined(_MSC_VER)
    /* cause MSVC does not know 'zu' when the compiler(MSBuild) does... */
    #pragma warning(disable: 4455)
    constexpr std::size_t operator"" zu(std::size_t size) {
        return size;
    }
#endif

#endif