/*
    @copyright gurotopia (c) 2024-05-25
    @version perent SHA: c4f5b9e7b1d948eaed42550930addb6a4f9ee765 2026-1-29
*/
#include "include/pch.hpp"
#include "include/event_type/__event_type.hpp"

#include "include/database/shouhin.hpp" // @note init_shouhin_tachi()
#include "include/https/https.hpp" // @note https::listener()
#include "include/https/server_data.hpp" // @note g_server_data
#include <filesystem>
#include <csignal>

int main()
{
    /* !! please press Ctrl + C when restarting or stopping server !! */
    std::signal(SIGINT, safe_disconnect_peers);
#ifdef SIGHUP // @note unix
    std::signal(SIGHUP,  safe_disconnect_peers); // @note PuTTY, SSH problems
#endif

    /* libary version checker */
    std::printf("ZTzTopia/enet %d.%d.%d\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    std::printf("sqlite/sqlite3 %s\n", sqlite3_libversion());
    std::printf("openssl/openssl %s\n", OpenSSL_version(OPENSSL_VERSION_STRING));
    
    std::filesystem::create_directory("db");
    init_shouhin_tachi();
    g_server_data = init_server_data();

    enet_initialize();
    {
        ENetAddress address{
            .type = ENET_ADDRESS_TYPE_IPV4, 
            .port = g_server_data.port
        };

        host = enet_host_create (ENET_ADDRESS_TYPE_IPV4, &address, 50zu/* max peer count */, 2zu, 0, 0);
        std::thread(&https::listener).detach();
    } // @note delete server_data, address
    host->usingNewPacketForServer = true;
    host->checksum = enet_crc32;
    enet_host_compress_with_range_coder(host);

    try // @note for people who don't use a debugger···
    {
        const int size = std::filesystem::file_size("items.dat");
        im_data = compress_state(::state{
            .type = 0x10, 
            .peer_state = 0x08, 
            .size = size
        });

        im_data.resize(im_data.size() + size); // @note resize to fit binary data
        std::ifstream("items.dat", std::ios::binary)
            .read(reinterpret_cast<char*>(&im_data[sizeof(::state)]), size); // @note the binary data···

        cache_items();
    } // @note delete size
    catch (std::filesystem::filesystem_error error) { puts(error.what()); }
    catch (...) { puts("unknown error occured during decoding items.dat"); } // @note if this appears, it's probably cache_items()···

    ENetEvent event{};
    while (true)
        while (enet_host_service(host, &event, 100/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);

    return 0;
}
