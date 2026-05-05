#pragma once
#ifndef PCH_HPP
#define PCH_HPP

    #include "enet/enet.h" // @version SHA: 2b22def89210ca86b729a22a94a60bbacc9667f2 25-03-22 | https://github.com/ZTzTopia/enet

    #include <algorithm>
    #include <chrono>
    #include <thread>
    #include <vector>
    #include <openssl/ssl.h>

    #include "proton/Variant.hpp"
    
    #include "database/items.hpp"
    #include "database/peer.hpp"
    #include "database/world.hpp"

    #include "tools/string.hpp"

#endif