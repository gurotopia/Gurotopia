#pragma once
#ifndef PCH_HPP
#define PCH_HPP

    #include "enet/enet.h" // @version SHA: 2b22def89210ca86b729a22a94a60bbacc9667f2 25-03-22 | https://github.com/ZTzTopia/enet

    #include <cstring>
    #include <unordered_map>
    #include <vector>
    #include <algorithm>
    #include <ranges>
    #include <chrono>
    #include <thread>
    #include <fstream>
    #include <format>
    #include <sqlite3.h>
    #include <openssl/ssl.h>

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

    /* @todo have this compile in files of use */
    constexpr std::byte PACKET_CREATE{ 04 };
    constexpr std::byte PACKET_STATE{ 0x08 };

#endif