#pragma once

#include "renderer/types.hpp"

#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

struct world;

namespace renderer {
    rgba8 rgba_from_item_color(std::uint32_t color);
    rgba8 tile_flag_color(const tile_view& tile);
    rgba8 rainbow_color(u_int x, u_int y);

    void fill_image(image_rgba& img, rgba8 color);
    void tile_background(image_rgba& dst, const texture_rgba& src);
    void blit_scaled(image_rgba& dst, const texture_rgba& src);
    void blit_tile_tinted(image_rgba& dst, const texture_rgba& src, u_int src_x, u_int src_y, int dest_x, int dest_y, u_int size, const rgba8* tint, bool flip_x);
    void blit_tile_shadow(image_rgba& dst, const texture_rgba& src, u_int src_x, u_int src_y, int dest_x, int dest_y, u_int size, bool flip_x, u_char alpha);
    void blit_tile_alpha(image_rgba& dst, const texture_rgba& src, u_int src_x, u_int src_y, int dest_x, int dest_y, u_int size, u_char alpha);
    void blit_scaled_region_tinted(image_rgba& dst, const texture_rgba& src, u_int src_x, u_int src_y, u_int src_w, u_int src_h, int dest_x, int dest_y, u_int dest_w, u_int dest_h, const rgba8* tint);
    void blit_scaled_region_shadow(image_rgba& dst, const texture_rgba& src, u_int src_x, u_int src_y, u_int src_w, u_int src_h, int dest_x, int dest_y, u_int dest_w, u_int dest_h, u_char alpha);
    void draw_text(image_rgba& img, int x, int y, std::string_view text, rgba8 color);

    std::vector<u_char> encode_png_rgba(const image_rgba& img);

    std::pair<u_int,u_int> resolve_tile_src_coords(const texture_rgba& texture, int texture_x, int texture_y, int offset_x, int offset_y);
    std::pair<int,int> default_drop_texture_xy(const render_item& item);
    std::pair<int,int> choose_visual_background(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage);
    std::pair<int,int> choose_visual_foreground(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage);
    std::pair<u_int,u_int> resolve_flag_src_coords(const tile_map_view& map, std::span<const tile_view> tiles, std::size_t index, u_char flag);

    struct weather_backdrop {
        rgba8 base_color;
        std::string texture;
        std::string layer_prefix;
    };

    weather_backdrop default_backdrop();
    std::vector<std::string> resolve_backdrop_layers(const std::filesystem::path& textures_root, const std::string& texture_name);
    bool should_scale_backdrop(const std::string& name, const texture_rgba& src, const image_rgba& dst);

    u_char effective_weather_id(const ::world& w);
    tile_map_view build_tile_map(const ::world& w, std::vector<tile_view>& scratch);
}
