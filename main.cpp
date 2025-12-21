/*
    @copyright gurotopia (c) 2024-05-25
    @version perent SHA: 4f2cc02ee2aac4b895b9386652865059ffe21e7f 2025-12-21
*/
#include "include/pch.hpp"
#include "include/event_type/__event_type.hpp"

#include "include/database/shouhin.hpp"
#include "include/tools/string.hpp"
#include "include/https/https.hpp"
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
    printf("ZTzTopia/enet %d.%d.%d\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    printf("sqlite/sqlite3 %s\n", sqlite3_libversion());
    printf("openssl/openssl %s\n", OpenSSL_version(OPENSSL_VERSION_STRING));
    
    std::filesystem::create_directory("db");
    init_shouhin_tachi();

    enet_initialize();
    {
        ::server_data server_data = init_server_data();
        ENetAddress address{
            .type = ENET_ADDRESS_TYPE_IPV4, 
            .port = server_data.port
        };

        server = enet_host_create (ENET_ADDRESS_TYPE_IPV4, &address, 50zu/* max peer count */, 2zu, 0, 0);
        std::thread(&https::listener, server_data).detach();
    } // @note delete server_data, address
    server->usingNewPacketForServer = true;
    server->checksum = enet_crc32;
    enet_host_compress_with_range_coder(server);

    try // @note for people who don't use a debugger..
    {
        const uintmax_t size = std::filesystem::file_size("items.dat");

        im_data.resize(im_data.size() + size); // @note state + items.dat
        im_data[0zu] = PACKET_CREATE; // @note 04 00 00 00
        im_data[4zu] = std::byte{ 0x10 }; // @note 16 00 00 00
        /* {...} */
        im_data[16zu] = PACKET_STATE; // @note 08 00 00 00
        *reinterpret_cast<std::uintmax_t*>(&im_data[sizeof(::state)]) = size; // @note at the end of ::state,  items.dat size and data

        std::ifstream("items.dat", std::ios::binary)
            .read(reinterpret_cast<char*>(&im_data[60zu]), size);

        printf("storing items.dat in binary; %zu KB of stack memory\n", im_data.size() / 1024);

        cache_items();
    } // @note delete size
    catch (std::filesystem::filesystem_error error) { puts(error.what()); }
    catch (...) { puts("unknown error occured during decoding items.dat"); } // @note if this appears, it's probably cache_items()...

    ENetEvent event{};
    while (true)
        while (enet_host_service(server, &event, 100/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);
    return 0;
}
