#include "renderer/textures.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iterator>
#include <tuple>

#include "third_party/miniz.h"
#include "third_party/stb_image.h"

namespace renderer {
    std::string normalize_key(std::string_view in) {
        std::string out;
        out.reserve(in.size());
        for (char ch : in) {
            if (ch == '\\') ch = '/';
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return out;
    }

    void texture_store::index_dir(const std::filesystem::path& root) {
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

    void texture_store::build_index(const std::filesystem::path& textures_root, const std::filesystem::path& cache_root) {
        if (indexed) return;
        index_dir(textures_root);
        index_dir(cache_root);
        indexed = true;
    }

    std::filesystem::path texture_store::resolve_path(std::string_view texture_name) const {
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
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f) return {};
            const std::streamsize size = f.tellg();
            if (size <= 0) return {};
            std::vector<u_char> bytes(static_cast<std::size_t>(size));
            f.seekg(0, std::ios::beg);
            if (!f.read(reinterpret_cast<char*>(bytes.data()), size)) return {};
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

    // Backdrop helpers live in renderer/blit.cpp
}
