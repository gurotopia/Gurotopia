#include "pch.hpp"
#include "renderworld.hpp"
#include "weather.hpp"
#include "tools/create_dialog.hpp"

#include "renderer/types.hpp"
#include "renderer/io.hpp"
#include "renderer/textures.hpp"
#include "renderer/blit.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <algorithm>
#include <iostream>

using namespace renderer;

namespace {
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

    const std::string cache_url = "https://pub-e5089e1bb0404a2e8a914b7da82071d8.r2.dev/cache.zip";
    if (cache_ready()) {
        g_cache_state.store(2);
    } else {
        const int state = g_cache_state.load();
        if (state == 1) {
            packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "`6Cache is still downloading... try again shortly.``", 0u, 1u });
            packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Render cache is still downloading... try again shortly." });
            packet::action(*event.peer, "log", "msg|`6Render cache still downloading...``");
            return;
        }
        if (state == 3 && !cache_ready()) {
            packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "`4Cache download failed. Check server console.``", 0u, 1u });
            packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Render cache download failed. Check server console/URL." });
            packet::action(*event.peer, "log", "msg|`4Render cache download failed.``");
            std::cout << "[renderworld] cache download failed (state=failed). Check URL or permissions.\n";
            return;
        }
        ensure_cache_async(cache_url);
        packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, "`6Please wait while the cache is being downloaded for the first time... Try /renderworld again in a moment.``", 0u, 1u });
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Downloading render cache for the first time... try again shortly." });
        std::cout << "[renderworld] cache missing, starting background download.\n";
        return;
    }

    const auto cache_paths = resolve_cache_paths();
    const std::filesystem::path textures_root = cache_paths.textures_root;
    const std::filesystem::path cache_root = cache_paths.cache_root;
    if (textures_root.empty() || !std::filesystem::exists(textures_root)) {
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
