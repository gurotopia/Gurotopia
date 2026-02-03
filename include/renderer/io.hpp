#pragma once

#include "renderer/types.hpp"

#include <atomic>
#include <filesystem>
#include <cstring>
#include <string>
#include <string_view>

namespace renderer {
    struct reader {
        const u_char* data{};
        std::size_t size{};
        std::size_t pos{};

        bool can_read(std::size_t n) const;
        template<typename T>
        T read() {
            if (!can_read(sizeof(T))) return T{};
            T val{};
            std::memcpy(&val, data + pos, sizeof(T));
            pos += sizeof(T);
            return val;
        }
        std::string_view read_string_i16_lossy();
        void skip_bytes(std::size_t n);
    };

    struct mapped_file {
        const u_char* data{};
        std::size_t size{};
#ifdef _WIN32
        void* file_handle{};
        void* map_handle{};
#endif
        bool open(const std::filesystem::path& path);
        void close();
        ~mapped_file();
    };

    struct render_item_db {
        std::vector<render_item> items;
        bool loaded{};
        std::uint16_t version{};
        mapped_file mapped{};

        bool load_from_file(const std::filesystem::path& path);
        const render_item* get(u_short id) const;
    };

    cache_paths resolve_cache_paths();
    bool cache_ready();
    bool verify_and_sync_cache(const std::string& r2_zip_url);

    extern std::atomic<int> g_cache_state; // 0 idle, 1 downloading, 2 ready, 3 failed
    void ensure_cache_async(const std::string& url);
}
