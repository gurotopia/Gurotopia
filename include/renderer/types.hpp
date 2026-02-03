#pragma once

#include "pch.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace renderer {
    inline constexpr int kWorldWidth = 100;
    inline constexpr int kTileSize = 32;

    inline constexpr u_char TILEFLAG_FLIPPED = 0x01;
    inline constexpr u_char TILEFLAG_GLUE = 0x02;
    inline constexpr u_char TILEFLAG_WATER = 0x04;
    inline constexpr u_char TILEFLAG_FIRE = 0x08;
    inline constexpr u_char TILEFLAG_PAINT_RED = 0x10;
    inline constexpr u_char TILEFLAG_PAINT_GREEN = 0x20;
    inline constexpr u_char TILEFLAG_PAINT_BLUE = 0x40;
    inline constexpr u_char TILEFLAG_ENABLED = 0x80;

    inline constexpr u_char STORAGE_SINGLE_FRAME_ALONE = 0;
    inline constexpr u_char STORAGE_SINGLE_FRAME = 1;
    inline constexpr u_char STORAGE_SMART_EDGE = 2;
    inline constexpr u_char STORAGE_SMART_EDGE_HORIZ = 3;
    inline constexpr u_char STORAGE_SMART_CLING = 4;
    inline constexpr u_char STORAGE_SMART_OUTER = 5;
    inline constexpr u_char STORAGE_RANDOM = 6;
    inline constexpr u_char STORAGE_SMART_EDGE_VERT = 7;
    inline constexpr u_char STORAGE_SMART_EDGE_HORIZ_CAVE = 8;
    inline constexpr u_char STORAGE_SMART_CLING2 = 9;
    inline constexpr u_char STORAGE_SMART_EDGE_HORIZ_FLIPPABLE = 10;

    inline constexpr u_short ITEM_ID_BLANK = 0;

    inline constexpr int SHADOW_OFFSET_X = -4;
    inline constexpr int SHADOW_OFFSET_Y = 4;
    inline constexpr u_char SHADOW_ALPHA = 143;
    inline constexpr u_char OVERLAY_ALPHA = 145;
    inline constexpr u_int DROP_ICON_SIZE = 19;
    inline constexpr u_int DROP_BOX_SIZE = 20;
    inline constexpr u_int SEED_ICON_SIZE = 16;

    inline constexpr std::array<u_short, 23> LOCK_ITEM_IDS = {
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

    struct cache_paths {
        std::filesystem::path textures_root;
        std::filesystem::path cache_root;
    };
}
