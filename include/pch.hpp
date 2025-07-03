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
    #include "sqlite/sqlite3.h" // @version SHA: a67c71224f5821547040b637aad7cddf4ef0778a (25/06/24) | https://github.com/sqlite/sqlite
    #include "enet/enet.h" // @version SHA: 2b22def89210ca86b729a22a94a60bbacc9667f2 (25/03/22) | https://github.com/ZTzTopia/enet
    #include "ssl/openssl/ssl.h" // @version SHA: 7bdc0d13d2b9ce1c1d0ec1f89dacc16e5d045314 (25/06/26) | https://github.com/openssl/openssl

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