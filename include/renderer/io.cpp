#include "renderer/io.hpp"

#include <cstring>
#include <format>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include "third_party/miniz.h"

namespace renderer {
    bool reader::can_read(std::size_t n) const {
        return data && pos + n <= size;
    }

    std::string_view reader::read_string_i16_lossy() {
        const std::int16_t len = read<std::int16_t>();
        if (len <= 0) return {};
        if (!can_read(static_cast<std::size_t>(len))) {
            pos = size;
            return {};
        }
        const auto* start = reinterpret_cast<const char*>(data + pos);
        pos += static_cast<std::size_t>(len);
        return std::string_view(start, static_cast<std::size_t>(len));
    }

    void reader::skip_bytes(std::size_t n) {
        if (can_read(n)) pos += n; else pos = size;
    }

    bool mapped_file::open(const std::filesystem::path& path) {
#ifdef _WIN32
        file_handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER file_size{};
        if (!GetFileSizeEx(static_cast<HANDLE>(file_handle), &file_size)) {
            CloseHandle(static_cast<HANDLE>(file_handle));
            file_handle = nullptr;
            return false;
        }
        map_handle = CreateFileMappingW(static_cast<HANDLE>(file_handle), nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!map_handle) {
            CloseHandle(static_cast<HANDLE>(file_handle));
            file_handle = nullptr;
            return false;
        }
        void* view = MapViewOfFile(static_cast<HANDLE>(map_handle), FILE_MAP_READ, 0, 0, 0);
        if (!view) {
            CloseHandle(static_cast<HANDLE>(map_handle));
            CloseHandle(static_cast<HANDLE>(file_handle));
            map_handle = nullptr;
            file_handle = nullptr;
            return false;
        }
        data = static_cast<const u_char*>(view);
        size = static_cast<std::size_t>(file_size.QuadPart);
        return true;
#else
        (void)path;
        return false;
#endif
    }

    void mapped_file::close() {
#ifdef _WIN32
        if (data) UnmapViewOfFile(data);
        if (map_handle) CloseHandle(static_cast<HANDLE>(map_handle));
        if (file_handle && file_handle != INVALID_HANDLE_VALUE) CloseHandle(static_cast<HANDLE>(file_handle));
        data = nullptr;
        size = 0;
        map_handle = nullptr;
        file_handle = nullptr;
#endif
    }

    mapped_file::~mapped_file() { close(); }

    bool render_item_db::load_from_file(const std::filesystem::path& path) {
        if (!mapped.open(path)) return false;
        if (mapped.size < 6) return false;
        reader r{ mapped.data, mapped.size, 0 };
        version = r.read<std::uint16_t>();
        const std::int32_t count = r.read<std::int32_t>();
        if (count <= 0) return false;
        items.clear();
        items.resize(static_cast<std::size_t>(count));

        for (std::int32_t index = 0; index < count; ++index) {
            render_item it{};
            const std::int32_t id = r.read<std::int32_t>();
            r.read<std::uint16_t>();
            r.read<u_char>();
            r.read<u_char>();
            {
                std::int16_t name_len = r.read<std::int16_t>();
                if (name_len > 0) r.skip_bytes(static_cast<std::size_t>(name_len));
            }
            it.texture = r.read_string_i16_lossy();
            r.read<std::uint32_t>();
            r.read<u_char>();
            r.read<std::uint32_t>();
            it.tex_x = static_cast<int>(r.read<u_char>());
            it.tex_y = static_cast<int>(r.read<u_char>());
            it.tile_storage = r.read<u_char>();
            r.read<u_char>();
            r.read<u_char>();
            r.read<u_char>();
            r.read<std::uint32_t>();
            r.read<u_char>();
            r.read<std::int16_t>();
            r.read<u_char>();

            r.read_string_i16_lossy();
            r.read<std::uint32_t>();
            r.read<std::uint32_t>();

            r.read_string_i16_lossy();
            r.read_string_i16_lossy();
            r.read_string_i16_lossy();
            r.read_string_i16_lossy();

            it.seed_base = r.read<u_char>();
            it.seed_over = r.read<u_char>();
            r.read<u_char>();
            r.read<u_char>();
            it.seed_color = r.read<std::uint32_t>();
            it.tree_color = r.read<std::uint32_t>();
            r.read<std::int32_t>();
            r.read<std::int32_t>();

            r.read<std::uint32_t>();
            r.read_string_i16_lossy();
            r.read_string_i16_lossy();
            r.read_string_i16_lossy();
            r.read<std::uint32_t>();
            r.read<std::uint32_t>();
            r.read<std::int32_t>();
            r.skip_bytes(60);
            r.read<std::uint32_t>();
            r.read<std::uint32_t>();

            if (version >= 11) r.read_string_i16_lossy();
            if (version >= 12) {
                r.read<std::uint32_t>();
                r.skip_bytes(9);
            }
            if (version >= 13) r.read<std::uint32_t>();
            if (version >= 14) r.read<std::uint32_t>();
            if (version >= 15) {
                r.read<u_char>();
                r.read<std::uint32_t>();
                r.read<std::uint32_t>();
                r.read<std::uint32_t>();
                r.read<std::uint32_t>();
                r.read<std::uint32_t>();
                r.read<std::uint32_t>();
                r.read_string_i16_lossy();
            }
            if (version >= 16) r.read_string_i16_lossy();
            if (version >= 17) r.read<std::int32_t>();
            if (version >= 18) r.read<std::uint32_t>();
            if (version >= 19) r.skip_bytes(9);
            if (version >= 21) r.read<std::uint16_t>();
            if (version >= 22) r.read_string_i16_lossy();
            if (version >= 23) r.read<std::uint32_t>();
            if (version >= 24) r.read<u_char>();

            if (id >= 0 && id < count) {
                it.id = static_cast<u_short>(id);
                it.valid = !it.texture.empty();
                items[static_cast<std::size_t>(id)] = std::move(it);
            }
        }
        loaded = true;
        return true;
    }

    const render_item* render_item_db::get(u_short id) const {
        const std::size_t idx = static_cast<std::size_t>(id);
        if (idx >= items.size()) return nullptr;
        const render_item& it = items[idx];
        return it.valid ? &it : nullptr;
    }

    std::string_view strip_cache_prefix(std::string_view name) {
        constexpr std::string_view p1 = "render_cache/";
        constexpr std::string_view p2 = "render_cache\\";
        constexpr std::string_view p3 = "Cache/";
        constexpr std::string_view p4 = "Cache\\";
        if (name.size() >= p1.size() && name.substr(0, p1.size()) == p1) return name.substr(p1.size());
        if (name.size() >= p2.size() && name.substr(0, p2.size()) == p2) return name.substr(p2.size());
        if (name.size() >= p3.size() && name.substr(0, p3.size()) == p3) return name.substr(p3.size());
        if (name.size() >= p4.size() && name.substr(0, p4.size()) == p4) return name.substr(p4.size());
        return name;
    }

    bool is_safe_zip_path(std::string_view name) {
        if (name.empty()) return false;
        if (name.find("..") != std::string_view::npos) return false;
        if (name.find(':') != std::string_view::npos) return false;
        if (name.front() == '/' || name.front() == '\\') return false;
        return true;
    }

    cache_paths resolve_cache_paths() {
        const std::filesystem::path a_textures = "render_cache/GTCache";
        const std::filesystem::path a_cache = "render_cache/cache";
        if (std::filesystem::exists(a_textures) && !std::filesystem::is_empty(a_textures)) {
            return { a_textures, a_cache };
        }
        const std::filesystem::path b_textures = "render_cache/Cache/GTCache";
        const std::filesystem::path b_cache = "render_cache/Cache/cache";
        if (std::filesystem::exists(b_textures) && !std::filesystem::is_empty(b_textures)) {
            return { b_textures, b_cache };
        }
        return {};
    }

    bool cache_ready() {
        const auto paths = resolve_cache_paths();
        return !paths.textures_root.empty();
    }

    bool verify_and_sync_cache(const std::string& r2_zip_url) {
        const std::filesystem::path cache_root = "render_cache";
        const std::filesystem::path target_dir = cache_root / "GTCache";
        const std::filesystem::path alt_dir = cache_root / "cache";

        if (std::filesystem::exists(target_dir) && !std::filesystem::is_empty(target_dir) &&
            std::filesystem::exists(alt_dir) && !std::filesystem::is_empty(alt_dir)) {
            return true;
        }

        std::error_code ec;
        std::filesystem::create_directories(cache_root, ec);
        if (ec) return false;

        const std::filesystem::path zip_path = cache_root / "cache.zip";
        const std::string cmd = std::format("curl -L -s -o \"{}\" \"{}\"", zip_path.string(), r2_zip_url);
        if (std::system(cmd.c_str()) != 0) {
            return cache_ready();
        }

        mz_zip_archive zip{};
        if (!mz_zip_reader_init_file(&zip, zip_path.string().c_str(), 0)) {
            std::filesystem::remove(zip_path, ec);
            return false;
        }

        const mz_uint files = mz_zip_reader_get_num_files(&zip);
        for (mz_uint i = 0; i < files; ++i) {
            mz_zip_archive_file_stat st{};
            if (!mz_zip_reader_file_stat(&zip, i, &st)) continue;
            std::string_view name = st.m_filename ? st.m_filename : "";
            name = strip_cache_prefix(name);
            if (!is_safe_zip_path(name)) continue;

            const std::filesystem::path out_path = cache_root / std::filesystem::path(std::string(name));
            if (st.m_is_directory) {
                std::filesystem::create_directories(out_path, ec);
                continue;
            }
            if (out_path.has_parent_path()) {
                std::filesystem::create_directories(out_path.parent_path(), ec);
            }
            mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0);
        }
        mz_zip_reader_end(&zip);
        std::filesystem::remove(zip_path, ec);

        return cache_ready();
    }

    std::atomic<int> g_cache_state{0};

    void ensure_cache_async(const std::string& url) {
        int expected = 0;
        if (!g_cache_state.compare_exchange_strong(expected, 1)) return;
        std::thread([url]{
            const bool ok = verify_and_sync_cache(url);
            g_cache_state.store(ok ? 2 : 3);
        }).detach();
    }
}
