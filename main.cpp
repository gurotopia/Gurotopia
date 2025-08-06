/*
    @copyright gurotopia (c) 2024-05-25
    @version perent SHA: 92cc449c618b4ad745af7fd0ff420fdfa5fc9adf 2025-08-06
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
    std::signal(SIGHUP,  safe_disconnect_peers); // @note itty, SSH problems
#endif

    /* libary version checker */
    printf("microsoft/mimalloc beta-%d\n", MI_MALLOC_VERSION);
    printf("ZTzTopia/enet %d.%d.%d\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    printf("sqlite/sqlite3 %s\n", sqlite3_sourceid());
    printf("openssl/openssl %s\n", OpenSSL_version(OPENSSL_VERSION_STRING));
    
    std::filesystem::create_directory("db");
    init_shouhin_tachi();

    {
        ENetCallbacks callbacks{
            .malloc = &mi_malloc,
            .free = &mi_free,
            .no_memory = []() { puts("ENet memory overflow"); }
        };
        enet_initialize_with_callbacks(ENET_VERSION, &callbacks);
    } // @note delete callbacks
    {
        ::_server_data server_data = init_server_data();
        ENetAddress address{
            .type = ENET_ADDRESS_TYPE_IPV4, 
            .port = server_data.port
        };

        server = enet_host_create (ENET_ADDRESS_TYPE_IPV4, &address, 50zu, 2zu, 0, 0);
        std::thread(&https::listener, server_data).detach();
    } // @note delete server_data, address
    server->usingNewPacketForServer = true;
    server->checksum = enet_crc32;
    enet_host_compress_with_range_coder(server);

    try // @note for people who don't use a debugger..
    {
        const uintmax_t size = std::filesystem::file_size("items.dat");

        im_data.resize(im_data.size() + size + 1/*@todo*/); // @note state + items.dat
        im_data[0zu] = std::byte{ 04 }; // @note 04 00 00 00
        im_data[4zu] = std::byte{ 0x10 }; // @note 16 00 00 00
        im_data[16zu] = std::byte{ 0x08 }; // @note 08 00 00 00
        *reinterpret_cast<std::uintmax_t*>(&im_data[56zu]) = size;

        std::ifstream("items.dat", std::ios::binary)
            .read(reinterpret_cast<char*>(&im_data[60zu]), size);

        cache_items();
    } // @note delete size
    catch (std::filesystem::filesystem_error error) { puts(error.what()); }
    catch (...) { puts("unknown error occured during decoding items.dat"); } // @note if this appears, it's probably cache_items()...

    ENetEvent event{};
    while (true)
        while (enet_host_service(server, &event, 1/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);
    return 0;
}
