/*
    @copyright gurotopia (c) 25-6-2024
    @author @leeendl | Lead Contributor, English Comments

    Project has open arms for contribution!

    looking for:
    - Indonesian translator
    - reverse engineer expert
*/
#include "include/pch.hpp"
#include "include/database/items.hpp"
#include "include/mimalloc/mimalloc.h" // @note https://github.com/microsoft/mimalloc
#include "include/network/compress.hpp" // @note isalzman's compressor
#include "include/database/peer.hpp"
#include "include/event_type/__event_type.hpp"

#include <filesystem>

int main()
{
    if (!std::filesystem::exists("worlds")) std::filesystem::create_directory("worlds");
    if (!std::filesystem::exists("players")) std::filesystem::create_directory("players");
    {
        ENetCallbacks callbacks{
            .malloc = &mi_malloc,
            .free = &mi_free,
            .no_memory = []() { printf("\e[1;31mENet memory overflow\e[0m\n"); }
        };
        if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks) == 0)
            printf("\e[38;5;247mENet initialize success! (v%d.%d.%d)\e[0m\n", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
        else
            printf("\e[1;31mENet initialize failed.\e[0m\n");
    } // @note delete callbacks
    server = enet_host_create({
        .host = in6addr_any,
        .port = 17091
    },
    50/*@note less to allocate than setting to MAX*/, 2);
    
    server->checksum = enet_crc32;
    enet_host_compress_with_range_coder(server);
    {
        std::uintmax_t size = std::filesystem::file_size("items.dat");

        im_data.resize(im_data.size() + size); // @note state + items.dat
        im_data[0] = std::byte{ 04 }; // @note 04 00 00 00
        im_data[4] = std::byte{ 0x10 }; // @note 16 00 00 00
        im_data[16] = std::byte{ 0x08 }; // @note 08 00 00 00
        *reinterpret_cast<std::uintmax_t*>(&im_data[56]) = size;

        std::ifstream("items.dat", std::ios::binary)
            .read(reinterpret_cast<char*>(&im_data[60]), size);
    } // @note delete size and close file
    cache_items();

    ENetEvent event{};
    while (true)
        while (enet_host_service(server, &event, 50/*ms*/) > 0)
            if (const auto i = event_pool.find(event.type); i != event_pool.end())
                i->second(event);
    return 0;
}
