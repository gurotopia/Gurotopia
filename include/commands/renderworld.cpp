#include "pch.hpp"
#include "renderworld.hpp"
#include "weather.hpp"
#include "tools/create_dialog.hpp"

#include <filesystem>
#include <format>
#include <array>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>
#include <cmath>
#include <tuple>
#include <string_view>
#include <span>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <immintrin.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "third_party/miniz.h"
#include "third_party/stb_image.h"

namespace {
    constexpr int kWorldWidth = 100;
    constexpr int kTileSize = 32;

    constexpr u_char TILEFLAG_FLIPPED = 0x01;
    constexpr u_char TILEFLAG_GLUE = 0x02;
    constexpr u_char TILEFLAG_WATER = 0x04;
    constexpr u_char TILEFLAG_FIRE = 0x08;
    constexpr u_char TILEFLAG_PAINT_RED = 0x10;
    constexpr u_char TILEFLAG_PAINT_GREEN = 0x20;
    constexpr u_char TILEFLAG_PAINT_BLUE = 0x40;
    constexpr u_char TILEFLAG_ENABLED = 0x80;

    constexpr u_char STORAGE_SINGLE_FRAME_ALONE = 0;
    constexpr u_char STORAGE_SINGLE_FRAME = 1;
    constexpr u_char STORAGE_SMART_EDGE = 2;
    constexpr u_char STORAGE_SMART_EDGE_HORIZ = 3;
    constexpr u_char STORAGE_SMART_CLING = 4;
    constexpr u_char STORAGE_SMART_OUTER = 5;
    constexpr u_char STORAGE_RANDOM = 6;
    constexpr u_char STORAGE_SMART_EDGE_VERT = 7;
    constexpr u_char STORAGE_SMART_EDGE_HORIZ_CAVE = 8;
    constexpr u_char STORAGE_SMART_CLING2 = 9;
    constexpr u_char STORAGE_SMART_EDGE_HORIZ_FLIPPABLE = 10;

    constexpr u_short ITEM_ID_BLANK = 0;

    constexpr int SHADOW_OFFSET_X = -4;
    constexpr int SHADOW_OFFSET_Y = 4;
    constexpr u_char SHADOW_ALPHA = 143;
    constexpr u_char OVERLAY_ALPHA = 145;
    constexpr u_int DROP_ICON_SIZE = 19;
    constexpr u_int DROP_BOX_SIZE = 20;
    constexpr u_int SEED_ICON_SIZE = 16;

    const std::array<u_short, 23> LOCK_ITEM_IDS = {
        202, 204, 206, 242, 1796, 2408, 2950, 4428, 4802, 4994, 5260, 5814,
        5980, 7188, 8470, 9640, 10410, 11550, 11586, 11902, 12654, 13200, 13636
    };

    struct rgba8 {
        u_char r{}, g{}, b{}, a{255};
    };

    struct image_rgba {
        int w{};
        int h{};
        std::vector<u_char> rgba;
    };

    struct texture_rgba {
        int w{};
        int h{};
        std::vector<u_char> rgba;
    };

    struct tile_view {
        u_short fg{};
        u_short bg{};
        u_char flags{};
        u_int index{};
        const std::string* label{};

        bool has_flag(u_char f) const { return (flags & f) != 0; }
    };

    struct tile_map_view {
        int width{};
        int height{};
        std::span<const tile_view> tiles;

        const tile_view* get_tile(int x, int y) const {
            if (x < 0 || y < 0 || x >= width || y >= height) return nullptr;
            const int idx = y * width + x;
            return &tiles[static_cast<std::size_t>(idx)];
        }
    };

    struct render_arena {
        std::vector<u_char> image;
        std::vector<tile_view> tiles;
    };

    struct render_item {
        u_short id{};
        std::string_view texture;
        int tex_x{};
        int tex_y{};
        u_char tile_storage{};
        u_char seed_base{};
        u_char seed_over{};
        std::uint32_t seed_color{};
        std::uint32_t tree_color{};
        bool valid{};
    };

    inline std::uint64_t fnv1a_mix(std::uint64_t h, std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ull;
        return h;
    }

    std::uint64_t hash_world(const ::world& w) {
        std::uint64_t h = 1469598103934665603ull;
        h = fnv1a_mix(h, static_cast<std::uint64_t>(w.blocks.size()));
        for (const ::block& b : w.blocks) {
            std::uint64_t v = (static_cast<std::uint64_t>(static_cast<u_short>(b.fg))      ) |
                              (static_cast<std::uint64_t>(static_cast<u_short>(b.bg)) << 16) |
                              (static_cast<std::uint64_t>(b.state3)                 << 32) |
                              (static_cast<std::uint64_t>(b.state4)                 << 40);
            h = fnv1a_mix(h, v);
        }
        return h;
    }

    std::string sanitize_world_name(const std::string& name) {
        std::string out;
        out.reserve(name.size());
        for (char ch : name) {
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '_' || ch == '-') {
                out.push_back(ch);
            }
        }
        if (out.empty()) out = "world";
        return out;
    }

    std::string build_render_dialog(const std::string& path) {
        return ::create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wWorld Render``", 656)
            .add_spacer("small")
            .add_smalltext(std::format("`6Saved:`` {} ``", path))
            .end_dialog("render_world", "Close", "");
    }

// <SECTION:helpers_parse>
    struct reader {
        const u_char* data{};
        std::size_t size{};
        std::size_t pos{};

        bool can_read(std::size_t n) const {
            return data && pos + n <= size;
        }

        u_char read_u8() {
            return can_read(1) ? data[pos++] : 0;
        }

        std::uint16_t read_u16() {
            if (!can_read(2)) return 0;
            std::uint16_t v = static_cast<std::uint16_t>(data[pos]) |
                              (static_cast<std::uint16_t>(data[pos + 1]) << 8);
            pos += 2;
            return v;
        }

        std::int16_t read_i16() {
            return static_cast<std::int16_t>(read_u16());
        }

        std::uint32_t read_u32() {
            if (!can_read(4)) return 0;
            std::uint32_t v = static_cast<std::uint32_t>(data[pos]) |
                              (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
                              (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
                              (static_cast<std::uint32_t>(data[pos + 3]) << 24);
            pos += 4;
            return v;
        }

        std::int32_t read_i32() {
            return static_cast<std::int32_t>(read_u32());
        }

        std::string_view read_string_i16_lossy() {
            std::int16_t len = read_i16();
            if (len <= 0) return {};
            if (!can_read(static_cast<std::size_t>(len))) {
                pos = size;
                return {};
            }
            const auto* start = reinterpret_cast<const char*>(data + pos);
            pos += static_cast<std::size_t>(len);
            return std::string_view(start, static_cast<std::size_t>(len));
        }

        void skip_bytes(std::size_t n) {
            if (can_read(n)) pos += n; else pos = size;
        }
    };

    struct mapped_file {
        const u_char* data{};
        std::size_t size{};
#ifdef _WIN32
        HANDLE file_handle{};
        HANDLE map_handle{};
#endif
        bool open(const std::filesystem::path& path) {
#ifdef _WIN32
            file_handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file_handle == INVALID_HANDLE_VALUE) return false;
            LARGE_INTEGER file_size{};
            if (!GetFileSizeEx(file_handle, &file_size)) {
                CloseHandle(file_handle);
                file_handle = nullptr;
                return false;
            }
            map_handle = CreateFileMappingW(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if (!map_handle) {
                CloseHandle(file_handle);
                file_handle = nullptr;
                return false;
            }
            void* view = MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, 0);
            if (!view) {
                CloseHandle(map_handle);
                CloseHandle(file_handle);
                map_handle = nullptr;
                file_handle = nullptr;
                return false;
            }
            data = static_cast<const u_char*>(view);
            size = static_cast<std::size_t>(file_size.QuadPart);
            return true;
#else
            return false;
#endif
        }
        void close() {
#ifdef _WIN32
            if (data) UnmapViewOfFile(data);
            if (map_handle) CloseHandle(map_handle);
            if (file_handle && file_handle != INVALID_HANDLE_VALUE) CloseHandle(file_handle);
            data = nullptr;
            size = 0;
            map_handle = nullptr;
            file_handle = nullptr;
#endif
        }
        ~mapped_file() { close(); }
    };

    struct render_item_db {
        std::vector<render_item> items;
        bool loaded{};
        std::uint16_t version{};
        mapped_file mapped{};

        bool load_from_file(const std::filesystem::path& path) {
            if (!mapped.open(path)) return false;
            if (mapped.size < 6) return false;
            reader r{ mapped.data, mapped.size, 0 };
            version = r.read_u16();
            const std::int32_t count = r.read_i32();
            if (count <= 0) return false;
            items.clear();
            items.resize(static_cast<std::size_t>(count));

            for (std::int32_t index = 0; index < count; ++index) {
                render_item it{};
                const std::int32_t id = r.read_i32();
                r.read_u16();
                r.read_u8();
                r.read_u8();
                {
                    std::int16_t name_len = r.read_i16();
                    if (name_len > 0) r.skip_bytes(static_cast<std::size_t>(name_len));
                }
                it.texture = r.read_string_i16_lossy();
                r.read_u32();
                r.read_u8();
                r.read_u32();
                it.tex_x = static_cast<int>(r.read_u8());
                it.tex_y = static_cast<int>(r.read_u8());
                it.tile_storage = r.read_u8();
                r.read_u8();
                r.read_u8();
                r.read_u8();
                r.read_u32();
                r.read_u8();
                r.read_i16();
                r.read_u8();

                r.read_string_i16_lossy();
                r.read_u32();
                r.read_u32();

                r.read_string_i16_lossy();
                r.read_string_i16_lossy();
                r.read_string_i16_lossy();
                r.read_string_i16_lossy();

                it.seed_base = r.read_u8();
                it.seed_over = r.read_u8();
                r.read_u8();
                r.read_u8();
                it.seed_color = r.read_u32();
                it.tree_color = r.read_u32();
                r.read_i32();
                r.read_i32();

                r.read_u32();
                r.read_string_i16_lossy();
                r.read_string_i16_lossy();
                r.read_string_i16_lossy();
                r.read_u32();
                r.read_u32();
                r.read_i32();
                r.skip_bytes(60);
                r.read_u32();
                r.read_u32();

                if (version >= 11) r.read_string_i16_lossy();
                if (version >= 12) {
                    r.read_u32();
                    r.skip_bytes(9);
                }
                if (version >= 13) r.read_u32();
                if (version >= 14) r.read_u32();
                if (version >= 15) {
                    r.read_u8();
                    r.read_u32();
                    r.read_u32();
                    r.read_u32();
                    r.read_u32();
                    r.read_u32();
                    r.read_u32();
                    r.read_string_i16_lossy();
                }
                if (version >= 16) r.read_string_i16_lossy();
                if (version >= 17) r.read_i32();
                if (version >= 18) r.read_u32();
                if (version >= 19) r.skip_bytes(9);
                if (version >= 21) r.read_u16();
                if (version >= 22) r.read_string_i16_lossy();
                if (version >= 23) r.read_u32();
                if (version >= 24) r.read_u8();

                if (id >= 0 && id < count) {
                    it.id = static_cast<u_short>(id);
                    it.valid = !it.texture.empty();
                    items[static_cast<std::size_t>(id)] = std::move(it);
                }
            }
            loaded = true;
            return true;
        }

        const render_item* get(u_short id) const {
            const std::size_t idx = static_cast<std::size_t>(id);
            if (idx >= items.size()) return nullptr;
            const render_item& it = items[idx];
            return it.valid ? &it : nullptr;
        }
    };

// <SECTION:helpers_textures>
    std::string normalize_key(std::string_view in) {
        std::string out;
        out.reserve(in.size());
        for (char ch : in) {
            if (ch == '\\') ch = '/';
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return out;
    }

    struct texture_store {
        std::unordered_map<std::string, std::filesystem::path> index;
        std::unordered_map<std::string, std::shared_ptr<texture_rgba>> cache;
        std::unordered_set<std::string> failed;
        bool indexed{};

        void index_dir(const std::filesystem::path& root) {
            if (!std::filesystem::exists(root)) return;
            for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it) {
                const auto& entry = *it;
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                if (ext != ".rttex" && ext != ".png" && ext != ".webp") continue;

                const std::string file_name = entry.path().filename().string();
                const std::string file_key = normalize_key(file_name);
                if (!index.contains(file_key)) index.emplace(file_key, entry.path());

                std::error_code ec;
                auto rel = std::filesystem::relative(entry.path(), root, ec);
                if (!ec) {
                    const std::string rel_key = normalize_key(rel.generic_string());
                    if (!index.contains(rel_key)) index.emplace(rel_key, entry.path());
                }
            }
        }

        void build_index(const std::filesystem::path& textures_root, const std::filesystem::path& cache_root) {
            if (indexed) return;
            index_dir(textures_root);
            index_dir(cache_root);
            indexed = true;
        }

        std::filesystem::path resolve_path(std::string_view texture_name) const {
            const std::string key = normalize_key(texture_name);
            if (auto it = index.find(key); it != index.end()) return it->second;

            std::filesystem::path p{ std::string(texture_name) };
            if (auto it = index.find(normalize_key(p.filename().string())); it != index.end()) return it->second;

            const auto stem = p.stem().string();
            if (!stem.empty()) {
                const std::string base = normalize_key((p.parent_path() / stem).generic_string());
                for (const char* ext : { ".rttex", ".png", ".webp" }) {
                    std::string candidate = base + ext;
                    if (auto it = index.find(candidate); it != index.end()) return it->second;
                }
            }

            return {};
        }
    };

    std::uint32_t read_le_u32(const std::vector<u_char>& data, std::size_t offset) {
        if (offset + 4 > data.size()) return 0;
        return static_cast<std::uint32_t>(data[offset]) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
               (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }

    std::vector<u_char> decode_rttex_bytes(const std::vector<u_char>& bytes, int& out_w, int& out_h) {
        std::vector<u_char> data;
        if (bytes.size() >= 6 && std::memcmp(bytes.data(), "RTPACK", 6) == 0) {
            if (bytes.size() < 32) return {};
            const std::uint32_t unpacked = read_le_u32(bytes, 12);
            if (unpacked == 0) return {};

            const std::size_t zlib_start = 32;
            if (zlib_start >= bytes.size()) return {};

            std::vector<u_char> out;
            out.resize(unpacked);
            mz_ulong out_len = static_cast<mz_ulong>(out.size());
            const mz_ulong src_len = static_cast<mz_ulong>(bytes.size() - zlib_start);
            const int res = mz_uncompress(out.data(), &out_len, bytes.data() + zlib_start, src_len);
            if (res != MZ_OK) return {};
            out.resize(static_cast<std::size_t>(out_len));
            data = std::move(out);
        } else {
            data = bytes;
        }

        if (data.size() < 16 || std::memcmp(data.data(), "RTTXTR", 6) != 0) return {};
        out_w = static_cast<int>(read_le_u32(data, 8));
        out_h = static_cast<int>(read_le_u32(data, 12));
        if (out_w <= 0 || out_h <= 0) return {};

        const std::size_t pixel_len = static_cast<std::size_t>(out_w) * static_cast<std::size_t>(out_h) * 4u;
        if (data.size() < pixel_len) return {};
        const std::size_t header_size = data.size() - pixel_len;
        const u_char* pixels = data.data() + header_size;

        std::vector<u_char> flipped(pixel_len);
        const std::size_t row = static_cast<std::size_t>(out_w) * 4u;
        for (int y = 0; y < out_h; ++y) {
            const std::size_t src = static_cast<std::size_t>(y) * row;
            const std::size_t dst = static_cast<std::size_t>(out_h - 1 - y) * row;
            std::memcpy(flipped.data() + dst, pixels + src, row);
        }
        return flipped;
    }

    std::shared_ptr<texture_rgba> load_texture_from_path(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (ext == ".rttex") {
            std::ifstream f(path, std::ios::binary);
            if (!f) return {};
            std::vector<u_char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            int w = 0, h = 0;
            auto rgba = decode_rttex_bytes(bytes, w, h);
            if (rgba.empty()) return {};
            auto tex = std::make_shared<texture_rgba>();
            tex->w = w;
            tex->h = h;
            tex->rgba = std::move(rgba);
            return tex;
        }

        int w = 0, h = 0, comp = 0;
        unsigned char* data = stbi_load(path.string().c_str(), &w, &h, &comp, 4);
        if (!data || w <= 0 || h <= 0) {
            if (data) stbi_image_free(data);
            return {};
        }
        auto tex = std::make_shared<texture_rgba>();
        tex->w = w;
        tex->h = h;
        tex->rgba.assign(data, data + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
        stbi_image_free(data);
        return tex;
    }

    std::shared_ptr<texture_rgba> get_texture(texture_store& store, const std::filesystem::path& textures_root, const std::filesystem::path& cache_root, std::string_view name) {
        if (name.empty()) return {};
        const std::string key = normalize_key(name);
        if (auto it = store.cache.find(key); it != store.cache.end()) return it->second;
        if (store.failed.contains(key)) return {};

        store.build_index(textures_root, cache_root);
        const std::filesystem::path resolved = store.resolve_path(name);
        if (resolved.empty()) {
            store.failed.insert(key);
            return {};
        }

        auto tex = load_texture_from_path(resolved);
        if (!tex) {
            store.failed.insert(key);
            return {};
        }
        store.cache.emplace(key, tex);
        return tex;
    }

// <SECTION:helpers_render>
    inline void blend_pixel(u_char* dst, const u_char* src) {
        const u_char sa = src[3];
        if (sa == 0) return;
        if (sa == 255) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = 255;
            return;
        }
        const int inv = 255 - sa;
        dst[0] = static_cast<u_char>((src[0] * sa + dst[0] * inv) / 255);
        dst[1] = static_cast<u_char>((src[1] * sa + dst[1] * inv) / 255);
        dst[2] = static_cast<u_char>((src[2] * sa + dst[2] * inv) / 255);
        dst[3] = 255;
    }

    inline bool copy_4px_if_opaque(u_char* dst, const u_char* src) {
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
        const __m128i alpha = _mm_and_si128(_mm_srli_epi32(v, 24), _mm_set1_epi32(0xFF));
        const __m128i cmp = _mm_cmpeq_epi32(alpha, _mm_set1_epi32(0xFF));
        if (_mm_movemask_epi8(cmp) == 0xFFFF) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), v);
            return true;
        }
#endif
        return false;
    }

    inline rgba8 rgba_from_item_color(std::uint32_t color) {
        u_char a = static_cast<u_char>(color & 0xFF);
        if (a == 0) a = 255;
        return rgba8{
            static_cast<u_char>((color >> 8) & 0xFF),
            static_cast<u_char>((color >> 16) & 0xFF),
            static_cast<u_char>((color >> 24) & 0xFF),
            a
        };
    }

    inline rgba8 tile_flag_color(const tile_view& tile) {
        const u_char r = tile.has_flag(TILEFLAG_PAINT_RED) ? 255 : 0;
        const u_char g = tile.has_flag(TILEFLAG_PAINT_GREEN) ? 255 : 0;
        const u_char b = tile.has_flag(TILEFLAG_PAINT_BLUE) ? 255 : 0;
        if (r == 0 && g == 0 && b == 0) return rgba8{255, 255, 255, 255};
        return rgba8{r, g, b, 255};
    }

    inline rgba8 rainbow_color(u_int x, u_int y) {
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const float time = static_cast<float>(millis) * 0.5f + static_cast<float>(x) + static_cast<float>(y);
        const auto r = static_cast<u_char>(std::sin(time * 0.0012f) * 127.0f + 128.0f);
        const auto g = static_cast<u_char>(std::sin(time * 0.0012f + 2.0f) * 127.0f + 128.0f);
        const auto b = static_cast<u_char>(std::sin(time * 0.0012f + 4.0f) * 127.0f + 128.0f);
        return rgba8{r, g, b, 255};
    }

    const u_char* font5x7(char c) {
        struct glyph { char ch; u_char rows[7]; };
        static const glyph glyphs[] = {
            {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
            {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
            {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
            {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
            {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
            {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
            {'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},
            {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
            {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
            {'J',{0x01,0x01,0x01,0x01,0x11,0x11,0x0E}},
            {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
            {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
            {'M',{0x11,0x1B,0x15,0x11,0x11,0x11,0x11}},
            {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
            {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
            {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
            {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
            {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
            {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
            {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
            {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
            {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
            {'W',{0x11,0x11,0x11,0x11,0x15,0x1B,0x11}},
            {'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
            {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
            {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
            {'a',{0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}},
            {'b',{0x10,0x10,0x16,0x19,0x11,0x11,0x1E}},
            {'c',{0x00,0x00,0x0E,0x11,0x10,0x11,0x0E}},
            {'d',{0x01,0x01,0x0D,0x13,0x11,0x11,0x0F}},
            {'e',{0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}},
            {'f',{0x06,0x08,0x1E,0x08,0x08,0x08,0x08}},
            {'g',{0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}},
            {'h',{0x10,0x10,0x16,0x19,0x11,0x11,0x11}},
            {'i',{0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}},
            {'j',{0x02,0x00,0x06,0x02,0x02,0x12,0x0C}},
            {'k',{0x10,0x10,0x11,0x12,0x1C,0x12,0x11}},
            {'l',{0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}},
            {'m',{0x00,0x00,0x1A,0x15,0x11,0x11,0x11}},
            {'n',{0x00,0x00,0x16,0x19,0x11,0x11,0x11}},
            {'o',{0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}},
            {'p',{0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}},
            {'q',{0x00,0x00,0x0F,0x11,0x0F,0x01,0x01}},
            {'r',{0x00,0x00,0x16,0x19,0x10,0x10,0x10}},
            {'s',{0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E}},
            {'t',{0x08,0x08,0x1E,0x08,0x08,0x08,0x06}},
            {'u',{0x00,0x00,0x11,0x11,0x11,0x13,0x0D}},
            {'v',{0x00,0x00,0x11,0x11,0x11,0x0A,0x04}},
            {'w',{0x00,0x00,0x11,0x11,0x15,0x1B,0x11}},
            {'x',{0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}},
            {'y',{0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}},
            {'z',{0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}},
            {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
            {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
            {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
            {'3',{0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},
            {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
            {'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
            {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
            {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
            {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
            {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
            {':',{0x00,0x04,0x00,0x00,0x04,0x00,0x00}},
            {'(',{0x02,0x04,0x08,0x08,0x08,0x04,0x02}},
            {')',{0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
            {'_',{0x00,0x00,0x00,0x00,0x00,0x00,0x1F}},
            {'-',{0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
            {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
        };
        for (const auto& g : glyphs) {
            if (g.ch == c) return g.rows;
        }
        return glyphs[0].rows;
    }

    void draw_glyph(image_rgba& img, int x, int y, char c, rgba8 color) {
        const u_char* rows = font5x7(c);
        for (int row = 0; row < 7; ++row) {
            if (y + row < 0 || y + row >= img.h) continue;
            const u_char bits = rows[row];
            for (int col = 0; col < 5; ++col) {
                if ((bits & (1 << (4 - col))) == 0) continue;
                const int px = x + col;
                if (px < 0 || px >= img.w) continue;
                u_char* dp = img.rgba.data() + (static_cast<std::size_t>(y + row) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(px)) * 4u;
                u_char pixel[4] = { color.r, color.g, color.b, color.a };
                blend_pixel(dp, pixel);
            }
        }
    }

    void draw_text(image_rgba& img, int x, int y, std::string_view text, rgba8 color) {
        int cursor = x;
        for (char c : text) {
            draw_glyph(img, cursor, y, c, color);
            cursor += 6;
        }
    }

    void fill_image(image_rgba& img, rgba8 color) {
        const std::size_t pixels = static_cast<std::size_t>(img.w) * static_cast<std::size_t>(img.h);
        img.rgba.resize(pixels * 4u);
        for (std::size_t i = 0; i < pixels; ++i) {
            const std::size_t o = i * 4u;
            img.rgba[o] = color.r;
            img.rgba[o + 1] = color.g;
            img.rgba[o + 2] = color.b;
            img.rgba[o + 3] = color.a;
        }
    }

    void tile_background(image_rgba& dst, const texture_rgba& src) {
        if (src.w <= 0 || src.h <= 0) return;
        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        for (int y = 0; y < dst.h; ++y) {
            const int sy = y % src.h;
            for (int x = 0; x < dst.w; ++x) {
                const int sx = x % src.w;
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, sp);
            }
        }
    }

    void blit_scaled(image_rgba& dst, const texture_rgba& src) {
        if (dst.w <= 0 || dst.h <= 0 || src.w <= 0 || src.h <= 0) return;
        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        for (int y = 0; y < dst.h; ++y) {
            const int sy = (y * src.h) / dst.h;
            for (int x = 0; x < dst.w; ++x) {
                const int sx = (x * src.w) / dst.w;
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, sp);
            }
        }
    }

    bool should_scale_backdrop(const std::string& name, const texture_rgba& src, const image_rgba& dst) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (lower.find("sky") != std::string::npos ||
            lower.find("layer") != std::string::npos ||
            lower.find("cloud") != std::string::npos ||
            lower.find("mountain") != std::string::npos ||
            lower.find("sun") != std::string::npos ||
            lower.find("background") != std::string::npos ||
            lower.find("foreground") != std::string::npos) {
            return true;
        }

        return src.w >= dst.w / 2 || src.h >= dst.h / 2;
    }

    void blit_tile_tinted(
        image_rgba& dst,
        const texture_rgba& src,
        u_int src_x,
        u_int src_y,
        int dest_x,
        int dest_y,
        u_int size,
        const rgba8* tint,
        bool flip_x
    ) {
        const int max_x = std::min(dest_x + static_cast<int>(size), dst.w);
        const int max_y = std::min(dest_y + static_cast<int>(size), dst.h);
        const int start_x = std::max(dest_x, 0);
        const int start_y = std::max(dest_y, 0);
        if (start_x >= max_x || start_y >= max_y) return;

        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        if (!tint && !flip_x) {
            for (int y = start_y; y < max_y; ++y) {
                const int rel_y = y - dest_y;
                if (rel_y < 0) continue;
                const u_int sy = src_y + static_cast<u_int>(rel_y);
                if (sy >= static_cast<u_int>(src.h)) continue;
                const std::size_t dst_row = static_cast<std::size_t>(y) * dst_stride;
                const std::size_t src_row = static_cast<std::size_t>(sy) * src_stride;
                int x = start_x;
                for (; x + 3 < max_x; x += 4) {
                    const int rel_x = x - dest_x;
                    if (rel_x < 0) continue;
                    const u_int sx = src_x + static_cast<u_int>(rel_x);
                    if (sx + 3 >= static_cast<u_int>(src.w)) break;
                    const u_char* sp = src.rgba.data() + src_row + static_cast<std::size_t>(sx) * 4u;
                    u_char* dp = dst.rgba.data() + dst_row + static_cast<std::size_t>(x) * 4u;
                    if (!copy_4px_if_opaque(dp, sp)) {
                        for (int i = 0; i < 4; ++i) blend_pixel(dp + static_cast<std::size_t>(i) * 4u, sp + static_cast<std::size_t>(i) * 4u);
                    }
                }
                for (; x < max_x; ++x) {
                    const int rel_x = x - dest_x;
                    if (rel_x < 0) continue;
                    const u_int sx = src_x + static_cast<u_int>(rel_x);
                    if (sx >= static_cast<u_int>(src.w)) continue;
                    const u_char* sp = src.rgba.data() + src_row + static_cast<std::size_t>(sx) * 4u;
                    if (sp[3] == 0) continue;
                    u_char* dp = dst.rgba.data() + dst_row + static_cast<std::size_t>(x) * 4u;
                    blend_pixel(dp, sp);
                }
            }
            return;
        }
        for (int y = start_y; y < max_y; ++y) {
            for (int x = start_x; x < max_x; ++x) {
                const int rel_x = x - dest_x;
                const int rel_y = y - dest_y;
                if (rel_x < 0 || rel_y < 0) continue;
                const int src_rel_x = flip_x ? static_cast<int>(size) - 1 - rel_x : rel_x;
                if (src_rel_x < 0) continue;
                const u_int sx = src_x + static_cast<u_int>(src_rel_x);
                const u_int sy = src_y + static_cast<u_int>(rel_y);
                if (sx >= static_cast<u_int>(src.w) || sy >= static_cast<u_int>(src.h)) continue;
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                u_char pixel[4] = { sp[0], sp[1], sp[2], sp[3] };
                if (tint) {
                    pixel[0] = static_cast<u_char>((pixel[0] * tint->r) / 255);
                    pixel[1] = static_cast<u_char>((pixel[1] * tint->g) / 255);
                    pixel[2] = static_cast<u_char>((pixel[2] * tint->b) / 255);
                }
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, pixel);
            }
        }
    }

    void blit_tile_shadow(
        image_rgba& dst,
        const texture_rgba& src,
        u_int src_x,
        u_int src_y,
        int dest_x,
        int dest_y,
        u_int size,
        bool flip_x,
        u_char alpha
    ) {
        const int max_x = std::min(dest_x + static_cast<int>(size), dst.w);
        const int max_y = std::min(dest_y + static_cast<int>(size), dst.h);
        const int start_x = std::max(dest_x, 0);
        const int start_y = std::max(dest_y, 0);
        if (start_x >= max_x || start_y >= max_y) return;

        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        for (int y = start_y; y < max_y; ++y) {
            for (int x = start_x; x < max_x; ++x) {
                const int rel_x = x - dest_x;
                const int rel_y = y - dest_y;
                if (rel_x < 0 || rel_y < 0) continue;
                const int src_rel_x = flip_x ? static_cast<int>(size) - 1 - rel_x : rel_x;
                if (src_rel_x < 0) continue;
                const u_int sx = src_x + static_cast<u_int>(src_rel_x);
                const u_int sy = src_y + static_cast<u_int>(rel_y);
                if (sx >= static_cast<u_int>(src.w) || sy >= static_cast<u_int>(src.h)) continue;
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                const u_char scaled_alpha = static_cast<u_char>((sp[3] * alpha) / 255);
                u_char shadow[4] = { 0, 0, 0, scaled_alpha };
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, shadow);
            }
        }
    }

    void blit_tile_alpha(
        image_rgba& dst,
        const texture_rgba& src,
        u_int src_x,
        u_int src_y,
        int dest_x,
        int dest_y,
        u_int size,
        u_char alpha
    ) {
        const int max_x = std::min(dest_x + static_cast<int>(size), dst.w);
        const int max_y = std::min(dest_y + static_cast<int>(size), dst.h);
        const int start_x = std::max(dest_x, 0);
        const int start_y = std::max(dest_y, 0);
        if (start_x >= max_x || start_y >= max_y) return;

        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        for (int y = start_y; y < max_y; ++y) {
            for (int x = start_x; x < max_x; ++x) {
                const int rel_x = x - dest_x;
                const int rel_y = y - dest_y;
                if (rel_x < 0 || rel_y < 0) continue;
                const u_int sx = src_x + static_cast<u_int>(rel_x);
                const u_int sy = src_y + static_cast<u_int>(rel_y);
                if (sx >= static_cast<u_int>(src.w) || sy >= static_cast<u_int>(src.h)) continue;
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                u_char overlay[4] = { sp[0], sp[1], sp[2], alpha };
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, overlay);
            }
        }
    }

    void blit_scaled_region_tinted(
        image_rgba& dst,
        const texture_rgba& src,
        u_int src_x,
        u_int src_y,
        u_int src_w,
        u_int src_h,
        int dest_x,
        int dest_y,
        u_int dest_w,
        u_int dest_h,
        const rgba8* tint
    ) {
        if (src_w == 0 || src_h == 0 || dest_w == 0 || dest_h == 0) return;
        if (src_x + src_w > static_cast<u_int>(src.w) || src_y + src_h > static_cast<u_int>(src.h)) return;
        const int max_x = std::min(dest_x + static_cast<int>(dest_w), dst.w);
        const int max_y = std::min(dest_y + static_cast<int>(dest_h), dst.h);
        const int start_x = std::max(dest_x, 0);
        const int start_y = std::max(dest_y, 0);
        if (start_x >= max_x || start_y >= max_y) return;

        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        if (!tint && src_w == dest_w && src_h == dest_h) {
            for (int y = start_y; y < max_y; ++y) {
                const int rel_y = y - dest_y;
                if (rel_y < 0) continue;
                const u_int sy = src_y + static_cast<u_int>(rel_y);
                if (sy >= static_cast<u_int>(src.h)) continue;
                const std::size_t dst_row = static_cast<std::size_t>(y) * dst_stride;
                const std::size_t src_row = static_cast<std::size_t>(sy) * src_stride;
                int x = start_x;
                for (; x + 3 < max_x; x += 4) {
                    const int rel_x = x - dest_x;
                    if (rel_x < 0) continue;
                    const u_int sx = src_x + static_cast<u_int>(rel_x);
                    if (sx + 3 >= static_cast<u_int>(src.w)) break;
                    const u_char* sp = src.rgba.data() + src_row + static_cast<std::size_t>(sx) * 4u;
                    u_char* dp = dst.rgba.data() + dst_row + static_cast<std::size_t>(x) * 4u;
                    if (!copy_4px_if_opaque(dp, sp)) {
                        for (int i = 0; i < 4; ++i) blend_pixel(dp + static_cast<std::size_t>(i) * 4u, sp + static_cast<std::size_t>(i) * 4u);
                    }
                }
                for (; x < max_x; ++x) {
                    const int rel_x = x - dest_x;
                    if (rel_x < 0) continue;
                    const u_int sx = src_x + static_cast<u_int>(rel_x);
                    if (sx >= static_cast<u_int>(src.w)) continue;
                    const u_char* sp = src.rgba.data() + src_row + static_cast<std::size_t>(sx) * 4u;
                    if (sp[3] == 0) continue;
                    u_char* dp = dst.rgba.data() + dst_row + static_cast<std::size_t>(x) * 4u;
                    blend_pixel(dp, sp);
                }
            }
            return;
        }
        for (int y = start_y; y < max_y; ++y) {
            for (int x = start_x; x < max_x; ++x) {
                const int rel_x = x - dest_x;
                const int rel_y = y - dest_y;
                if (rel_x < 0 || rel_y < 0) continue;
                const u_int sx = src_x + static_cast<u_int>((static_cast<u_int>(rel_x) * src_w) / dest_w);
                const u_int sy = src_y + static_cast<u_int>((static_cast<u_int>(rel_y) * src_h) / dest_h);
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                u_char pixel[4] = { sp[0], sp[1], sp[2], sp[3] };
                if (tint) {
                    pixel[0] = static_cast<u_char>((pixel[0] * tint->r) / 255);
                    pixel[1] = static_cast<u_char>((pixel[1] * tint->g) / 255);
                    pixel[2] = static_cast<u_char>((pixel[2] * tint->b) / 255);
                }
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, pixel);
            }
        }
    }

    void blit_scaled_region_shadow(
        image_rgba& dst,
        const texture_rgba& src,
        u_int src_x,
        u_int src_y,
        u_int src_w,
        u_int src_h,
        int dest_x,
        int dest_y,
        u_int dest_w,
        u_int dest_h,
        u_char alpha
    ) {
        if (src_w == 0 || src_h == 0 || dest_w == 0 || dest_h == 0) return;
        if (src_x + src_w > static_cast<u_int>(src.w) || src_y + src_h > static_cast<u_int>(src.h)) return;
        const int max_x = std::min(dest_x + static_cast<int>(dest_w), dst.w);
        const int max_y = std::min(dest_y + static_cast<int>(dest_h), dst.h);
        const int start_x = std::max(dest_x, 0);
        const int start_y = std::max(dest_y, 0);
        if (start_x >= max_x || start_y >= max_y) return;

        const std::size_t dst_stride = static_cast<std::size_t>(dst.w) * 4u;
        const std::size_t src_stride = static_cast<std::size_t>(src.w) * 4u;
        for (int y = start_y; y < max_y; ++y) {
            for (int x = start_x; x < max_x; ++x) {
                const int rel_x = x - dest_x;
                const int rel_y = y - dest_y;
                if (rel_x < 0 || rel_y < 0) continue;
                const u_int sx = src_x + static_cast<u_int>((static_cast<u_int>(rel_x) * src_w) / dest_w);
                const u_int sy = src_y + static_cast<u_int>((static_cast<u_int>(rel_y) * src_h) / dest_h);
                const u_char* sp = src.rgba.data() + (static_cast<std::size_t>(sy) * src_stride + static_cast<std::size_t>(sx) * 4u);
                if (sp[3] == 0) continue;
                const u_char scaled_alpha = static_cast<u_char>((sp[3] * alpha) / 255);
                u_char shadow[4] = { 0, 0, 0, scaled_alpha };
                u_char* dp = dst.rgba.data() + (static_cast<std::size_t>(y) * dst_stride + static_cast<std::size_t>(x) * 4u);
                blend_pixel(dp, shadow);
            }
        }
    }

    std::uint32_t crc32(const u_char* data, std::size_t len) {
        std::uint32_t crc = 0xFFFFFFFFu;
        for (std::size_t i = 0; i < len; ++i) {
            crc ^= data[i];
            for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
        return ~crc;
    }

    std::uint32_t adler32(const u_char* data, std::size_t len) {
        std::uint32_t a = 1, b = 0;
        constexpr std::uint32_t mod = 65521;
        for (std::size_t i = 0; i < len; ++i) {
            a += data[i];
            if (a >= mod) a -= mod;
            b += a;
            if (b >= mod) b -= mod;
        }
        return (b << 16) | a;
    }

    void append_be_u32(std::vector<u_char>& out, std::uint32_t v) {
        out.push_back(static_cast<u_char>((v >> 24) & 0xFF));
        out.push_back(static_cast<u_char>((v >> 16) & 0xFF));
        out.push_back(static_cast<u_char>((v >> 8) & 0xFF));
        out.push_back(static_cast<u_char>(v & 0xFF));
    }

    void append_chunk(std::vector<u_char>& out, const char type[4], const std::vector<u_char>& data) {
        append_be_u32(out, static_cast<std::uint32_t>(data.size()));
        const std::size_t type_pos = out.size();
        out.insert(out.end(), type, type + 4);
        out.insert(out.end(), data.begin(), data.end());

        std::uint32_t crc = crc32(reinterpret_cast<const u_char*>(out.data() + type_pos), 4 + data.size());
        append_be_u32(out, crc);
    }

    std::vector<u_char> encode_png_rgba(const image_rgba& img) {
        const int w = img.w;
        const int h = img.h;
        if (w <= 0 || h <= 0) return {};

        const std::size_t row_bytes = static_cast<std::size_t>(w) * 4u;
        std::vector<u_char> raw;
        raw.reserve((row_bytes + 1) * static_cast<std::size_t>(h));

        const u_char* src = img.rgba.data();
        for (int y = 0; y < h; ++y) {
            raw.push_back(0);
            const std::size_t offset = static_cast<std::size_t>(y) * row_bytes;
            raw.insert(raw.end(), src + offset, src + offset + row_bytes);
        }

        std::vector<u_char> zdata;
        zdata.reserve(raw.size() + 6 + (raw.size() / 65535 + 1) * 5);
        zdata.push_back(0x78);
        zdata.push_back(0x01);

        std::size_t remaining = raw.size();
        std::size_t cursor = 0;
        while (remaining > 0) {
            const u_short block = static_cast<u_short>(remaining > 65535 ? 65535 : remaining);
            const bool final = remaining <= 65535;
            zdata.push_back(static_cast<u_char>(final ? 0x01 : 0x00));
            zdata.push_back(static_cast<u_char>(block & 0xFF));
            zdata.push_back(static_cast<u_char>((block >> 8) & 0xFF));
            const u_short nlen = static_cast<u_short>(~block);
            zdata.push_back(static_cast<u_char>(nlen & 0xFF));
            zdata.push_back(static_cast<u_char>((nlen >> 8) & 0xFF));
            zdata.insert(zdata.end(), raw.begin() + static_cast<std::ptrdiff_t>(cursor),
                         raw.begin() + static_cast<std::ptrdiff_t>(cursor + block));
            cursor += block;
            remaining -= block;
        }

        const std::uint32_t adler = adler32(raw.data(), raw.size());
        append_be_u32(zdata, adler);

        std::vector<u_char> png;
        png.reserve(8 + 25 + zdata.size() + 12);
        const u_char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        png.insert(png.end(), sig, sig + 8);

        std::vector<u_char> ihdr;
        ihdr.reserve(13);
        append_be_u32(ihdr, static_cast<std::uint32_t>(w));
        append_be_u32(ihdr, static_cast<std::uint32_t>(h));
        ihdr.push_back(8);
        ihdr.push_back(6);
        ihdr.push_back(0);
        ihdr.push_back(0);
        ihdr.push_back(0);
        append_chunk(png, "IHDR", ihdr);
        append_chunk(png, "IDAT", zdata);
        append_chunk(png, "IEND", {});

        return png;
    }

    const std::array<int, 16> LUT4BIT = { 12, 11, 15, 8, 14, 7, 13, 2, 10, 9, 6, 4, 5, 3, 1, 0 };
    const std::array<int, 256> LUT8BIT = []{
        std::array<int, 256> lut{};
        lut[0] = 12; lut[2] = 11; lut[8] = 30; lut[10] = 44; lut[11] = 8; lut[16] = 29; lut[18] = 43; lut[22] = 7;
        lut[24] = 28; lut[26] = 42; lut[27] = 41; lut[30] = 40; lut[31] = 2; lut[64] = 10; lut[66] = 9; lut[72] = 46;
        lut[74] = 36; lut[75] = 35; lut[80] = 45; lut[82] = 33; lut[86] = 32; lut[88] = 39; lut[90] = 27; lut[91] = 23;
        lut[94] = 24; lut[95] = 18; lut[104] = 6; lut[106] = 34; lut[107] = 4; lut[120] = 38; lut[122] = 25; lut[123] = 20;
        lut[126] = 21; lut[127] = 16; lut[208] = 5; lut[210] = 31; lut[214] = 3; lut[216] = 37; lut[218] = 26; lut[219] = 22;
        lut[222] = 19; lut[223] = 15; lut[248] = 1; lut[250] = 17; lut[251] = 14; lut[254] = 13;
        return lut;
    }();

    const std::array<u_char, 256> LUT8BIT_FLAGS = []{
        std::array<u_char, 256> lut{};
        lut[0] = 12; lut[2] = 11; lut[8] = 30; lut[10] = 44; lut[11] = 8; lut[16] = 29; lut[18] = 43; lut[22] = 7;
        lut[24] = 28; lut[26] = 42; lut[27] = 41; lut[30] = 40; lut[31] = 2; lut[64] = 10; lut[66] = 9; lut[72] = 46;
        lut[74] = 36; lut[75] = 35; lut[80] = 45; lut[82] = 33; lut[86] = 32; lut[88] = 39; lut[90] = 27; lut[91] = 23;
        lut[94] = 24; lut[95] = 18; lut[104] = 6; lut[106] = 34; lut[107] = 4; lut[120] = 38; lut[122] = 25; lut[123] = 20;
        lut[126] = 21; lut[127] = 16; lut[208] = 5; lut[210] = 31; lut[214] = 3; lut[216] = 37; lut[218] = 26; lut[219] = 22;
        lut[222] = 19; lut[223] = 15; lut[248] = 1; lut[250] = 17; lut[251] = 14; lut[254] = 13;
        return lut;
    }();

    std::pair<int,int> choose_visual_offsets(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char tile_storage, bool use_foreground) {
        const int x = static_cast<int>(tile.index % map.width);
        const int y = static_cast<int>(tile.index / map.width);

        auto get_id = [&](const tile_view* t) { return use_foreground ? t->fg : t->bg; };
        auto same_or_glued = [&](const tile_view* t) {
            if (!t) return true;
            return get_id(t) == item_id || t->has_flag(TILEFLAG_GLUE);
        };

        switch (tile_storage) {
            case STORAGE_SMART_EDGE: {
                const tile_view* tl = map.get_tile(x - 1, y - 1);
                const tile_view* t  = map.get_tile(x, y - 1);
                const tile_view* tr = map.get_tile(x + 1, y - 1);
                const tile_view* l  = map.get_tile(x - 1, y);
                const tile_view* r  = map.get_tile(x + 1, y);
                const tile_view* bl = map.get_tile(x - 1, y + 1);
                const tile_view* b  = map.get_tile(x, y + 1);
                const tile_view* br = map.get_tile(x + 1, y + 1);

                bool b_tl = same_or_glued(tl);
                bool b_t = same_or_glued(t);
                bool b_tr = same_or_glued(tr);
                bool b_l = same_or_glued(l);
                bool b_r = same_or_glued(r);
                bool b_bl = same_or_glued(bl);
                bool b_b = same_or_glued(b);
                bool b_br = same_or_glued(br);

                if (!b_l || !b_t) b_tl = false;
                if (!b_l || !b_b) b_bl = false;
                if (!b_r || !b_t) b_tr = false;
                if (!b_r || !b_b) b_br = false;

                int bit = (b_tl ? 1 : 0) + (b_t ? 2 : 0) + (b_tr ? 4 : 0) + (b_l ? 8 : 0) + (b_r ? 16 : 0) + (b_bl ? 32 : 0) + (b_b ? 64 : 0) + (b_br ? 128 : 0);
                int offset_x = LUT8BIT[bit] % 8;
                int offset_y = LUT8BIT[bit] / 8;
                return { offset_x, offset_y };
            }
            case STORAGE_SMART_OUTER: {
                const tile_view* t = map.get_tile(x, y - 1);
                const tile_view* l = map.get_tile(x - 1, y);
                const tile_view* r = map.get_tile(x + 1, y);
                const tile_view* b = map.get_tile(x, y + 1);

                bool b_t = same_or_glued(t);
                bool b_l = same_or_glued(l);
                bool b_r = same_or_glued(r);
                bool b_b = same_or_glued(b);

                int bit = (b_t ? 1 : 0) + (b_l ? 2 : 0) + (b_r ? 4 : 0) + (b_b ? 8 : 0);
                int b_val = LUT4BIT[bit];
                int offset_x = b_val % 8;
                int offset_y = b_val / 8;
                return { offset_x, offset_y };
            }
            case STORAGE_SMART_EDGE_VERT: {
                const tile_view* t = map.get_tile(x, y - 1);
                const tile_view* b = map.get_tile(x, y + 1);
                bool b_t = same_or_glued(t);
                bool b_b = same_or_glued(b);
                int offset_x = (!b_t && !b_b) ? 3 : (!b_b && b_t) ? 0 : (b_b && !b_t) ? 2 : 1;
                return { offset_x, 0 };
            }
            case STORAGE_SMART_EDGE_HORIZ:
            case STORAGE_SMART_EDGE_HORIZ_CAVE:
            case STORAGE_SMART_EDGE_HORIZ_FLIPPABLE: {
                const tile_view* l = map.get_tile(x - 1, y);
                const tile_view* r = map.get_tile(x + 1, y);
                bool b_l = same_or_glued(l);
                bool b_r = same_or_glued(r);
                int offset_x = 1;
                if (!b_l && !b_r) offset_x = 3;
                else if (!b_l && b_r) offset_x = tile.has_flag(TILEFLAG_FLIPPED) ? 2 : 0;
                else if (b_l && !b_r) offset_x = tile.has_flag(TILEFLAG_FLIPPED) ? 0 : 2;
                return { offset_x, 0 };
            }
            case STORAGE_SMART_CLING: {
                const tile_view* t = map.get_tile(x, y - 1);
                const tile_view* l = map.get_tile(x - 1, y);
                const tile_view* r = map.get_tile(x + 1, y);
                const tile_view* b = map.get_tile(x, y + 1);

                auto diff_or_glued = [&](const tile_view* tt) {
                    if (!tt) return true;
                    const u_short id = get_id(tt);
                    return (id != 0 && id != item_id) || tt->has_flag(TILEFLAG_GLUE);
                };

                bool b_t = diff_or_glued(t);
                bool b_l = diff_or_glued(l);
                bool b_r = diff_or_glued(r);
                bool b_b = diff_or_glued(b);

                int offset_x = 4;
                if (b_l && !b_t && !b_b) offset_x = 0;
                if (b_t) offset_x = 1;
                if (b_r && !b_t && !b_b) offset_x = 2;
                if (b_b) offset_x = 3;
                return { offset_x, 0 };
            }
            case STORAGE_SMART_CLING2: {
                const tile_view* t = map.get_tile(x, y - 1);
                const tile_view* l = map.get_tile(x - 1, y);
                const tile_view* r = map.get_tile(x + 1, y);
                const tile_view* b = map.get_tile(x, y + 1);

                auto occupied = [&](const tile_view* tt) { return tt ? get_id(tt) != 0 : true; };
                bool b_t = occupied(t);
                bool b_l = occupied(l);
                bool b_r = occupied(r);
                bool b_b = occupied(b);

                int offset_x = 3;
                if (b_b) offset_x = 3;
                if (b_t && !b_b) offset_x = 1;
                if (b_l && !b_t && !b_b) offset_x = 0;
                if (b_r && !b_t && !b_b) offset_x = 2;
                return { offset_x, 0 };
            }
            case STORAGE_RANDOM: {
                std::uint32_t h = 0x9E3779B9u ^ (static_cast<std::uint32_t>(x) * 374761393u) ^ (static_cast<std::uint32_t>(y) * 668265263u);
                h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
                int offset_x = static_cast<int>(h % 4u);
                return { offset_x, 0 };
            }
            default:
                return { 0, 0 };
        }
    }

    inline std::pair<int,int> choose_visual_background(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage) {
        return choose_visual_offsets(map, tile, item_id, storage, false);
    }

    inline std::pair<int,int> choose_visual_foreground(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage) {
        return choose_visual_offsets(map, tile, item_id, storage, true);
    }

    std::pair<u_int,u_int> resolve_tile_src_coords(const texture_rgba& texture, int texture_x, int texture_y, int offset_x, int offset_y) {
        const int src_x = (texture_x + offset_x) * kTileSize;
        const int src_y = (texture_y + offset_y) * kTileSize;
        if (src_x < 0 || src_y < 0) return { 0, 0 };
        if (src_x + kTileSize > texture.w || src_y + kTileSize > texture.h) return { 0, 0 };
        return { static_cast<u_int>(src_x), static_cast<u_int>(src_y) };
    }

    std::pair<int,int> default_drop_texture_xy(const render_item& item) {
        int x = item.tex_x;
        int y = item.tex_y;
        switch (item.tile_storage) {
            case STORAGE_SMART_EDGE:
            case STORAGE_SMART_OUTER:
                x += 4; y += 1; break;
            case STORAGE_SMART_EDGE_HORIZ:
            case STORAGE_SMART_EDGE_VERT:
                x += 3; break;
            default: break;
        }
        return { x, y };
    }

    std::pair<u_int,u_int> resolve_flag_src_coords(const tile_map_view& map, std::span<const tile_view> tiles, std::size_t index, u_char flag) {
        const int width = map.width;
        const int height = map.height;
        if (width <= 0 || height <= 0) return { 0, 0 };
        const int x = static_cast<int>(index % static_cast<std::size_t>(width));
        const int y = static_cast<int>(index / static_cast<std::size_t>(width));
        const std::array<std::pair<int,int>, 8> neighbors = { {
            {-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}
        } };

        u_char bit = 0;
        const std::array<int,3> left = {0,3,5};
        const std::array<int,3> right = {2,4,6};

        for (int i = 0; i < 8; ++i) {
            const int nx = x + neighbors[static_cast<std::size_t>(i)].first;
            const int ny = y + neighbors[static_cast<std::size_t>(i)].second;
            const bool within = nx >= 0 && ny >= 0 && nx < width && ny < height;
            const bool is_edge = (std::find(left.begin(), left.end(), i) != left.end() && x == 0) ||
                                 (std::find(right.begin(), right.end(), i) != right.end() && x == width - 1);
            bool is_matching = false;
            if (within) {
                const tile_view& t = tiles[static_cast<std::size_t>(ny * width + nx)];
                is_matching = t.has_flag(flag);
            }
            if (is_edge || is_matching) bit |= static_cast<u_char>(1 << i);
        }

        if ((bit & 8) == 0 || (bit & 2) == 0) bit &= ~1;
        if ((bit & 16) == 0 || (bit & 2) == 0) bit &= ~4;
        if ((bit & 8) == 0 || (bit & 64) == 0) bit &= ~32;
        if ((bit & 16) == 0 || (bit & 64) == 0) bit &= ~128;

        const u_char offset = LUT8BIT_FLAGS[bit];
        const u_int ox = static_cast<u_int>(offset % 8) * kTileSize;
        const u_int oy = static_cast<u_int>(offset / 8) * kTileSize;
        return { ox, oy };
    }

    struct weather_backdrop {
        rgba8 base_color;
        std::string texture;
        std::string layer_prefix;
    };

    weather_backdrop default_backdrop() {
        return weather_backdrop{ rgba8{92, 138, 190, 255}, "game/bg_bdfc_gradient_sky.rttex", "bg_bdfc" };
    }

    std::string backdrop_prefix_from_texture(const std::string& texture_name) {
        std::filesystem::path p(texture_name);
        std::string stem = p.stem().string();
        if (stem.size() >= 4 && stem.ends_with("_sky")) return stem.substr(0, stem.size() - 4);
        auto pos = stem.find("_layer");
        if (pos != std::string::npos) return stem.substr(0, pos);
        pos = stem.find("_cloud");
        if (pos != std::string::npos) return stem.substr(0, pos);
        pos = stem.find("_mountain");
        if (pos != std::string::npos) return stem.substr(0, pos);
        pos = stem.find("_sun");
        if (pos != std::string::npos) return stem.substr(0, pos);
        return stem;
    }

    std::tuple<std::uint8_t, std::uint16_t, std::uint16_t> backdrop_layer_key(const std::string& name) {
        std::filesystem::path p(name);
        std::string stem = p.stem().string();
        std::string lower = stem;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        if (lower.find("_sky") != std::string::npos) return {0,0,0};

        auto idx = lower.find("_layer");
        if (idx != std::string::npos) {
            std::string rest = lower.substr(idx + 6);
            if (rest.empty()) return {1,0,0};
            auto pos = rest.find('_');
            std::uint16_t primary = 0, secondary = 0;
            try {
                primary = static_cast<std::uint16_t>(std::stoi(pos == std::string::npos ? rest : rest.substr(0,pos)));
                if (pos != std::string::npos) secondary = static_cast<std::uint16_t>(std::stoi(rest.substr(pos+1)));
            } catch (...) {}
            return {1, primary, secondary};
        }

        if (lower.find("background") != std::string::npos) return {2,0,0};
        if (lower.find("cloud") != std::string::npos) return {3,0,0};
        if (lower.find("mountain") != std::string::npos) return {4,0,0};
        if (lower.find("sun") != std::string::npos) return {5,0,0};
        if (lower.find("foreground") != std::string::npos) return {6,0,0};
        return {7,0,0};
    }

    std::vector<std::string> resolve_backdrop_layers(const std::filesystem::path& textures_root, const std::string& texture_name) {
        std::vector<std::string> layers;
        std::unordered_set<std::string> seen;

        auto push_unique = [&](const std::string& name) {
            if (!name.empty() && seen.insert(name).second) layers.push_back(name);
        };

        push_unique(texture_name);
        std::string prefix = backdrop_prefix_from_texture(texture_name);
        if (!prefix.empty()) {
            const std::array<const char*, 20> suffixes = {
                "_sky", "_layer1", "_layer1_1", "_layer1_2", "_layer2", "_layer2_1", "_layer2_2",
                "_layer3", "_layer4", "_layer5", "_layer6", "_layer7", "_layer8", "_sun",
                "_cloud", "_cloud1", "_cloud2", "_mountain", "_mountain1", "_mountain2"
            };
            for (auto suffix : suffixes) {
                for (auto ext : { ".rttex", ".png", ".webp" }) {
                    std::string name = prefix + suffix + ext;
                    if (std::filesystem::exists(textures_root / name)) {
                        push_unique(name);
                        break;
                    }
                }
            }
        }

        std::vector<std::string> extras;
        if (!prefix.empty()) {
            std::string pref_lower = prefix;
            std::transform(pref_lower.begin(), pref_lower.end(), pref_lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            for (auto& entry : std::filesystem::directory_iterator(textures_root)) {
                if (!entry.is_regular_file()) continue;
                const std::string file = entry.path().filename().string();
                std::string low = file;
                std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                if (low.rfind(pref_lower, 0) != 0) continue;
                if (!(low.ends_with(".rttex") || low.ends_with(".png") || low.ends_with(".webp"))) continue;
                if (seen.insert(file).second) extras.push_back(file);
            }
            std::sort(extras.begin(), extras.end(), [](const std::string& a, const std::string& b){
                return backdrop_layer_key(a) < backdrop_layer_key(b);
            });
            layers.insert(layers.end(), extras.begin(), extras.end());
        }

        return layers;
    }

    u_char effective_weather_id(const ::world& w) {
        for (const ::block& b : w.blocks) {
            if (items[b.fg].type == type::WEATHER_MACHINE && (b.state3 & S_TOGGLE)) {
                return static_cast<u_char>(get_weather_id(b.fg));
            }
        }
        return 0;
    }

    tile_map_view build_tile_map(const ::world& w, std::vector<tile_view>& scratch) {
        tile_map_view map{};
        map.width = kWorldWidth;
        map.height = static_cast<int>(w.blocks.size() / kWorldWidth);
        scratch.clear();
        scratch.reserve(w.blocks.size());

        u_int idx = 0;
        for (const ::block& b : w.blocks) {
            u_char flags = 0;
            if (b.state3 & S_LEFT) flags |= TILEFLAG_FLIPPED;
            if (b.state3 & S_TOGGLE) flags |= TILEFLAG_ENABLED;
            if (b.state4 & S_GLUE) flags |= TILEFLAG_GLUE;
            if (b.state4 & S_WATER) flags |= TILEFLAG_WATER;
            if (b.state4 & S_FIRE) flags |= TILEFLAG_FIRE;
            if (b.state4 & S_RED) flags |= TILEFLAG_PAINT_RED;
            if (b.state4 & S_GREEN) flags |= TILEFLAG_PAINT_GREEN;
            if (b.state4 & S_BLUE) flags |= TILEFLAG_PAINT_BLUE;

            tile_view t{};
            t.fg = static_cast<u_short>(b.fg);
            t.bg = static_cast<u_short>(b.bg);
            t.flags = flags;
            t.index = idx++;
            t.label = &b.label;
            scratch.push_back(t);
        }
        map.tiles = std::span<const tile_view>(scratch.data(), scratch.size());
        return map;
    }

// <SECTION:render_pipeline>
    image_rgba render_world_full(
        const ::world& w,
        render_item_db& db,
        texture_store& store,
        const std::filesystem::path& textures_root,
        const std::filesystem::path& cache_root,
        render_arena& arena
    ) {
        const int world_h = static_cast<int>(w.blocks.size() / kWorldWidth);
        const int width_px = kWorldWidth * kTileSize;
        const int height_px = world_h * kTileSize;

        image_rgba img{};
        img.w = width_px;
        img.h = height_px;
        arena.image.resize(static_cast<std::size_t>(width_px) * static_cast<std::size_t>(height_px) * 4u, 0);
        img.rgba.swap(arena.image);

        const tile_map_view tile_map = build_tile_map(w, arena.tiles);
        const u_char weather_id = effective_weather_id(w);

        const weather_backdrop backdrop = default_backdrop();
        fill_image(img, backdrop.base_color);

        auto weather_tex = get_texture(store, textures_root, cache_root, std::to_string(weather_id));
        if (weather_tex) {
            blit_scaled(img, *weather_tex);
        } else {
            auto base_tex = get_texture(store, textures_root, cache_root, backdrop.texture);
            if (base_tex) {
                for (const auto& layer : resolve_backdrop_layers(textures_root / "game", backdrop.texture)) {
                    auto tex = get_texture(store, textures_root, cache_root, layer);
                    if (!tex) continue;
                    if (should_scale_backdrop(layer, *tex, img)) blit_scaled(img, *tex);
                    else tile_background(img, *tex);
                }
            }
        }

        for (const auto& tile : tile_map.tiles) {
            if (tile.bg == ITEM_ID_BLANK) continue;
            const render_item* item = db.get(tile.bg);
            if (!item) continue;
            auto tex = get_texture(store, textures_root, cache_root, item->texture);
            if (!tex) continue;
            auto offsets = choose_visual_background(tile_map, tile, tile.bg, item->tile_storage);
            auto src = resolve_tile_src_coords(*tex, item->tex_x, item->tex_y, offsets.first, offsets.second);
            if (src.first == 0 && src.second == 0 && (item->tex_x + offsets.first != 0 || item->tex_y + offsets.second != 0)) continue;
            const int dx = static_cast<int>((tile.index % kWorldWidth) * kTileSize);
            const int dy = static_cast<int>((tile.index / kWorldWidth) * kTileSize);
            blit_tile_tinted(img, *tex, src.first, src.second, dx, dy, kTileSize, nullptr, false);
        }

        for (const auto& tile : tile_map.tiles) {
            if (tile.fg == ITEM_ID_BLANK) continue;
            const render_item* item = db.get(tile.fg);
            if (!item) continue;
            auto tex = get_texture(store, textures_root, cache_root, item->texture);
            if (!tex) continue;
            auto offsets = choose_visual_foreground(tile_map, tile, tile.fg, item->tile_storage);
            if (std::find(LOCK_ITEM_IDS.begin(), LOCK_ITEM_IDS.end(), item->id) != LOCK_ITEM_IDS.end()) {
                offsets = {2, 0};
            }
            auto src = resolve_tile_src_coords(*tex, item->tex_x, item->tex_y, offsets.first, offsets.second);
            const int dx = static_cast<int>((tile.index % kWorldWidth) * kTileSize) + SHADOW_OFFSET_X;
            const int dy = static_cast<int>((tile.index / kWorldWidth) * kTileSize) + SHADOW_OFFSET_Y;
            blit_tile_shadow(img, *tex, src.first, src.second, dx, dy, kTileSize, tile.has_flag(TILEFLAG_FLIPPED), SHADOW_ALPHA);
        }

        for (const auto& tile : tile_map.tiles) {
            if (tile.fg == ITEM_ID_BLANK) continue;
            const render_item* item = db.get(tile.fg);
            if (!item) continue;
            auto tex = get_texture(store, textures_root, cache_root, item->texture);
            if (!tex) continue;
            auto offsets = choose_visual_foreground(tile_map, tile, tile.fg, item->tile_storage);
            if (std::find(LOCK_ITEM_IDS.begin(), LOCK_ITEM_IDS.end(), item->id) != LOCK_ITEM_IDS.end()) {
                offsets = {2, 0};
            }
            auto src = resolve_tile_src_coords(*tex, item->tex_x, item->tex_y, offsets.first, offsets.second);
            const int dx = static_cast<int>((tile.index % kWorldWidth) * kTileSize);
            const int dy = static_cast<int>((tile.index / kWorldWidth) * kTileSize);
            rgba8 tint{};
            const rgba8* tint_ptr = nullptr;
            if (item->id == 2590) {
                tint = rainbow_color(static_cast<u_int>(dx), static_cast<u_int>(dy));
                tint_ptr = &tint;
            } else if (tile.flags & (TILEFLAG_PAINT_RED | TILEFLAG_PAINT_GREEN | TILEFLAG_PAINT_BLUE)) {
                tint = tile_flag_color(tile);
                tint_ptr = &tint;
            }
            blit_tile_tinted(img, *tex, src.first, src.second, dx, dy, kTileSize, tint_ptr, tile.has_flag(TILEFLAG_FLIPPED));
        }

        const auto water_tex = get_texture(store, textures_root, cache_root, "water");
        const auto fire_tex = get_texture(store, textures_root, cache_root, "fire");
        if (water_tex || fire_tex) {
            for (std::size_t i = 0; i < tile_map.tiles.size(); ++i) {
                const auto& tile = tile_map.tiles[i];
                const int dx = static_cast<int>((tile.index % kWorldWidth) * kTileSize);
                const int dy = static_cast<int>((tile.index / kWorldWidth) * kTileSize);
                if (tile.has_flag(TILEFLAG_WATER) && water_tex) {
                    auto src = resolve_flag_src_coords(tile_map, tile_map.tiles, i, TILEFLAG_WATER);
                    blit_tile_alpha(img, *water_tex, src.first, src.second, dx, dy, kTileSize, OVERLAY_ALPHA);
                }
                if (tile.has_flag(TILEFLAG_FIRE) && fire_tex) {
                    auto src = resolve_flag_src_coords(tile_map, tile_map.tiles, i, TILEFLAG_FIRE);
                    blit_tile_alpha(img, *fire_tex, src.first, src.second, dx, dy, kTileSize, OVERLAY_ALPHA);
                }
            }
        }

        for (const auto& [uid, obj] : w.ifloats) {
            if (obj.id == 0) continue;
            const render_item* item = db.get(static_cast<u_short>(obj.id));
            if (!item) continue;
            const int x = static_cast<int>(obj.pos.f_x());
            const int y = static_cast<int>(obj.pos.f_y());

            if (item->id % 2 == 0) {
                auto tex = get_texture(store, textures_root, cache_root, item->texture);
                if (!tex) continue;
                auto [sx, sy] = default_drop_texture_xy(*item);
                if (item->id == 112) {
                    if (obj.count == 5) sx += 1;
                    else if (obj.count == 10) sx += 2;
                    else if (obj.count == 50) sx += 3;
                    else if (obj.count == 100) sx += 4;
                }
                u_int src_x = static_cast<u_int>(sx * kTileSize);
                u_int src_y = static_cast<u_int>(sy * kTileSize);
                blit_scaled_region_shadow(img, *tex, src_x, src_y, kTileSize, kTileSize, x + SHADOW_OFFSET_X, y + SHADOW_OFFSET_Y, DROP_ICON_SIZE, DROP_ICON_SIZE, SHADOW_ALPHA);
                blit_scaled_region_tinted(img, *tex, src_x, src_y, kTileSize, kTileSize, x, y, DROP_ICON_SIZE, DROP_ICON_SIZE, nullptr);

                if (item->id != 112) {
                    auto box_tex = get_texture(store, textures_root, cache_root, "pickup_box");
                    if (box_tex) {
                        blit_scaled_region_shadow(img, *box_tex, 0, 0, DROP_BOX_SIZE, DROP_BOX_SIZE, x + SHADOW_OFFSET_X, y + SHADOW_OFFSET_Y, DROP_BOX_SIZE, DROP_BOX_SIZE, SHADOW_ALPHA);
                        blit_scaled_region_tinted(img, *box_tex, 0, 0, DROP_BOX_SIZE, DROP_BOX_SIZE, x, y, DROP_BOX_SIZE, DROP_BOX_SIZE, nullptr);
                    }
                }
            } else {
                auto seed_tex = get_texture(store, textures_root, cache_root, "seed");
                if (!seed_tex) continue;
                rgba8 base = rgba_from_item_color(item->seed_color);
                rgba8 overlay = item->tree_color != 0 ? rgba_from_item_color(item->tree_color) : base;

                u_int src_x = static_cast<u_int>(item->seed_base) * SEED_ICON_SIZE;
                u_int src_y = 0;
                blit_scaled_region_shadow(img, *seed_tex, src_x, src_y, SEED_ICON_SIZE, SEED_ICON_SIZE, x + SHADOW_OFFSET_X, y + SHADOW_OFFSET_Y, SEED_ICON_SIZE, SEED_ICON_SIZE, SHADOW_ALPHA);
                blit_scaled_region_tinted(img, *seed_tex, src_x, src_y, SEED_ICON_SIZE, SEED_ICON_SIZE, x, y, SEED_ICON_SIZE, SEED_ICON_SIZE, &base);

                if (item->seed_over != 0) {
                    const u_int over_x = static_cast<u_int>(item->seed_over) * SEED_ICON_SIZE;
                    const u_int over_y = SEED_ICON_SIZE;
                    blit_scaled_region_tinted(img, *seed_tex, over_x, over_y, SEED_ICON_SIZE, SEED_ICON_SIZE, x, y, SEED_ICON_SIZE, SEED_ICON_SIZE, &overlay);
                }
            }
        }

        const std::string watermark = "Renderworld made by: Gnome aka hmm (burg3r_1)";
        const int text_w = static_cast<int>(watermark.size()) * 6 - 1;
        const int text_h = 7;
        const int margin = 6;
        const int wx = std::max(0, img.w - text_w - margin);
        const int wy = std::max(0, img.h - text_h - margin);
        draw_text(img, wx + 1, wy + 1, watermark, rgba8{0, 0, 0, 180});
        draw_text(img, wx, wy, watermark, rgba8{255, 255, 255, 230});

        return img;
    }
}

// <SECTION:entrypoint>
void renderworld(ENetEvent& event, const std::string_view)
{
    ::peer* peer = static_cast<::peer*>(event.peer->data);
    if (!peer || !worlds.contains(peer->recent_worlds.back())) {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "`4You must be in a world to render it.``" });
        return;
    }

    ::world& w = worlds.at(peer->recent_worlds.back());
    const std::string safe_name = sanitize_world_name(w.name);
    const std::uint64_t version = hash_world(w);

    std::filesystem::path render_dir = "rendered_worlds";
    std::error_code ec;
    std::filesystem::create_directories(render_dir, ec);
    if (ec) {
        packet::action(*event.peer, "log", std::format("msg|`4Render failed: {}.``", ec.message()));
        return;
    }

    const std::filesystem::path png_path = render_dir / std::format("{}_render_v{:016x}.png", safe_name, version);
    if (std::filesystem::exists(png_path, ec) && !ec) {
        const std::string dialog = build_render_dialog(png_path.string());
        packet::create(*event.peer, false, 0, { "OnDialogRequest", dialog });
        return;
    }

    packet::action(*event.peer, "log", "msg|`6Rendering world, please wait...``");

    static render_item_db item_db;
    static texture_store tex_store;
    static render_arena arena;
    static bool item_db_loaded = false;

    if (!item_db_loaded) {
        if (!item_db.load_from_file("items.dat")) {
            packet::action(*event.peer, "log", "msg|`4Render failed: items.dat could not be parsed.``");
            return;
        }
        item_db_loaded = true;
    }

    const std::filesystem::path textures_root = "render_cache\\GTCache";
    const std::filesystem::path cache_root = "render_cache\\cache";
    if (!std::filesystem::exists(textures_root)) {
        packet::action(*event.peer, "log", "msg|`4Render failed: render_cache assets not found.``");
        return;
    }

    image_rgba img = render_world_full(w, item_db, tex_store, textures_root, cache_root, arena);
    const std::vector<u_char> png = encode_png_rgba(img);
    arena.image.swap(img.rgba);
    if (png.empty()) {
        packet::action(*event.peer, "log", "msg|`4Render failed: invalid image.``");
        return;
    }

    std::ofstream out(png_path, std::ios::binary);
    if (!out) {
        packet::action(*event.peer, "log", "msg|`4Render failed: could not open output file.``");
        return;
    }
    out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!out.good()) {
        packet::action(*event.peer, "log", "msg|`4Render failed: write error.``");
        return;
    }

    packet::action(*event.peer, "log", std::format("msg|`6Render saved to`` {} ``", png_path.string()));
    const std::string dialog = build_render_dialog(png_path.string());
    packet::create(*event.peer, false, 0, { "OnDialogRequest", dialog });
}
