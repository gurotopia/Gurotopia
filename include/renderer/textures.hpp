#pragma once

#include "renderer/types.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace renderer {
    struct texture_store {
        std::unordered_map<std::string, std::filesystem::path> index;
        std::unordered_map<std::string, std::shared_ptr<texture_rgba>> cache;
        std::unordered_set<std::string> failed;
        std::unordered_set<std::string> watermarked;
        bool indexed{};

        void index_dir(const std::filesystem::path& root);
        void build_index(const std::filesystem::path& textures_root, const std::filesystem::path& cache_root);
        std::filesystem::path resolve_path(std::string_view texture_name) const;
    };

    std::string normalize_key(std::string_view in);
    std::shared_ptr<texture_rgba> get_texture(texture_store& store, const std::filesystem::path& textures_root, const std::filesystem::path& cache_root, std::string_view name);

    // backdrop helpers live in blit.hpp/cpp to keep render pipeline cohesive
}
