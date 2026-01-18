#pragma once
#ifndef PCH_HPP
#define PCH_HPP

    #include "enet/enet.h" // @version SHA: 2b22def89210ca86b729a22a94a60bbacc9667f2 25-03-22 | https://github.com/ZTzTopia/enet

    #include <algorithm>
    #include <chrono>
    #include <fstream>
    #include <thread>
    #include <unordered_map>
    #include <vector>
    #include <sqlite3.h>
    #include <openssl/ssl.h>

    #include "database/items.hpp"
    #include "database/peer.hpp"
    #include "database/world.hpp"
    #include "packet/packet.hpp"

#endif