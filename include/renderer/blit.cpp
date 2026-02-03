#include "pch.hpp"
#include "commands/weather.hpp"
#include "renderer/blit.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <immintrin.h>
#endif

namespace renderer {
    inline u_char fast_div255(int v) {
        v += 1;
        return static_cast<u_char>((v + (v >> 8)) >> 8);
    }

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
        dst[0] = fast_div255(src[0] * sa + dst[0] * inv);
        dst[1] = fast_div255(src[1] * sa + dst[1] * inv);
        dst[2] = fast_div255(src[2] * sa + dst[2] * inv);
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

    rgba8 rgba_from_item_color(std::uint32_t color) {
        u_char a = static_cast<u_char>(color & 0xFF);
        if (a == 0) a = 255;
        return rgba8{
            static_cast<u_char>((color >> 8) & 0xFF),
            static_cast<u_char>((color >> 16) & 0xFF),
            static_cast<u_char>((color >> 24) & 0xFF),
            a
        };
    }

    rgba8 tile_flag_color(const tile_view& tile) {
        const u_char r = tile.has_flag(TILEFLAG_PAINT_RED) ? 255 : 0;
        const u_char g = tile.has_flag(TILEFLAG_PAINT_GREEN) ? 255 : 0;
        const u_char b = tile.has_flag(TILEFLAG_PAINT_BLUE) ? 255 : 0;
        if (r == 0 && g == 0 && b == 0) return rgba8{255, 255, 255, 255};
        return rgba8{r, g, b, 255};
    }

    rgba8 rainbow_color(u_int x, u_int y) {
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
    const std::array<int, 16> LUT4BIT_UDLR = []{
        std::array<int, 16> lut{};
        for (int mask = 0; mask < 16; ++mask) {
            const bool up = (mask & 1) != 0;
            const bool down = (mask & 2) != 0;
            const bool left = (mask & 4) != 0;
            const bool right = (mask & 8) != 0;
            const int legacy = (up ? 1 : 0) + (left ? 2 : 0) + (right ? 4 : 0) + (down ? 8 : 0);
            lut[mask] = LUT4BIT[legacy];
        }
        return lut;
    }();
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

                const int mask = (b_t ? 1 : 0) | (b_b ? 2 : 0) | (b_l ? 4 : 0) | (b_r ? 8 : 0);
                const int b_val = LUT4BIT_UDLR[mask];
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

    std::pair<int,int> choose_visual_background(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage) {
        return choose_visual_offsets(map, tile, item_id, storage, false);
    }

    std::pair<int,int> choose_visual_foreground(const tile_map_view& map, const tile_view& tile, u_short item_id, u_char storage) {
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
        return w.cached_weather_valid ? w.cached_weather_id : 0;
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


}
