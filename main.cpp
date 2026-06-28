/*
    @copyright gurotopia (c) 2024-05-25
    @version perent SHA: 8d25052c6e07513eac34449913c300bb37725a9a 2026-5-24
*/
#include "include/pch.hpp"
#include "include/event_type/__event_type.hpp"

#include "include/database/shouhin.hpp" // @note init_shouhin_tachi()
#include "include/https/https.hpp" // @note https::listener()
#include "include/https/server_data.hpp" // @note gServer_data
#include "include/database/database.hpp" // @note mysql_connect()
#include "include/database/database_config.hpp" // @note load_database_config(), gDatabase_config
#include "include/database/peer.hpp" // @note send_data, compress_state
#include "include/automate/holiday.hpp" // @note holiday
#include <filesystem>
#include <csignal>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

volatile sig_atomic_t gSignal = 0;
static void request_shutdown(sig_atomic_t signal) { gSignal = signal; }

int main()
{
    /* !! please press Ctrl + C when restarting or stopping server !! */
    std::signal(SIGINT, request_shutdown);
#ifdef SIGHUP // @note unix
    std::signal(SIGHUP, request_shutdown); // @note PuTTY, SSH problems
#endif

    /* libary version checker */
    std::printf("ZTzTopia/enet %d.%d.%d\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    std::printf("openssl/openssl %s\n", OpenSSL_version(OPENSSL_VERSION_STRING));

    enet_initialize();
    {
        gServer_data = init_server_data();
        ENetAddress address{
            .type = ENET_ADDRESS_TYPE_IPV4, 
            .port = gServer_data.port
        };

        host = enet_host_create (ENET_ADDRESS_TYPE_IPV4, &address, 50zu/* max peer count */, 2zu, 0, 0);
        std::thread(&https::listener).detach();
    } // @note delete address
    host->usingNewPacketForServer = true;
    host->checksum = enet_crc32;
    enet_host_compress_with_range_coder(host);

    gDatabase_config = load_database_config();
    mysql_connect();
    decode_items();      // @note reads items.dat into legible class members (id, item name, ect)
    parse_store();       // @todo thread loop this so the store can update without restarting server (stored in .\resource\store.txt)
    check_for_holiday(); // @note check for any holidays using local time (your VPS or local time) - @todo thread loop so it can change the holiday without restarting

    ENetEvent event{};
    auto last_ping = steady_clock::now();
    while (!gSignal)
    {
        // service ENet with 100ms timeout so ping fires without waiting 1s
        while (enet_host_service(host, &event, 100) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);

        // send PING_REQUEST to all peers in worlds every 5 seconds
        // without this, clients silently disconnect ~6s after entering a world
        auto now = steady_clock::now();
        if (now - last_ping >= 5s)
        {
            last_ping = now;
            for (ENetPeer &p : std::span(host->peers, host->peerCount))
            {
                if (p.state != ENET_PEER_STATE_CONNECTED) continue;
                ::peer *pp = static_cast<::peer*>(p.data);
                if (pp && pp->netid != 0) // @note peer is in a world
                {
                    send_data(p, compress_state(::state{.type = 0x16 /*PACKET_PING_REQUEST*/}));
                }
            }
        }
    }

    safe_disconnect_peers(gSignal);
    mysql_close(db);
    return 0;
}