/*
    @copyright gurotopia (c) 2024-05-25
    @version perent SHA: 8d25052c6e07513eac34449913c300bb37725a9a 2026-5-24
*/
#include "include/pch.hpp"
#include "include/event_type/__event_type.hpp"

#include "include/database/shouhin.hpp" // @note parse_store()
#include "include/https/https.hpp" // @note https::listener()
#include "include/https/server_data.hpp" // @note gServer_data
#include "include/database/database.hpp" // @note mysql_connect()
#include "include/database/database_config.hpp" // @note load_database_config(), gDatabase_config
#include "include/automate/holiday.hpp" // @note check_for_holiday(), holiday
#include <filesystem>
#include <csignal>
#include <atomic>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

volatile sig_atomic_t gSignal = 0;
static std::atomic<bool> gShutdown{false};
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
    parse_store();       // initial load
    check_for_holiday(); // initial load

    // thread: auto-refresh store every 60 seconds
    std::thread store_thread([]{
        while (!gShutdown)
        {
            std::this_thread::sleep_for(60s);
            if (gShutdown) break;
            parse_store();
            printf("[store] store refreshed from resources/store.txt\n");
        }
    });
    store_thread.detach();

    // thread: auto-refresh holiday every 300 seconds (5 min)
    std::thread holiday_thread([]{
        while (!gShutdown)
        {
            std::this_thread::sleep_for(300s);
            if (gShutdown) break;
            check_for_holiday();
        }
    });
    holiday_thread.detach();

    ENetEvent event{};
    while (!gSignal)
        while (enet_host_service(host, &event, 1000/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);

    gShutdown = true;
    safe_disconnect_peers(gSignal);
    mysql_close(db);
    return 0;
}
