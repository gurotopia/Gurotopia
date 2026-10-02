#include "pch.hpp"
#include "tools/bubble.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tools/string.hpp"
#include "tools/time.hpp"
#include "tools/random.hpp"
#include "database/database.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "blasts.hpp"

namespace
{
constexpr int W = 100, H = 60, BEDROCK_ROW = 54;
using heights = std::array<int, W>;

bool chance(int percent) { return RandomRange(0, 100) < percent; }
bool inside(int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; }
::block &at(::world &w, int x, int y) { return w.blocks[cord(x, y)]; }
short or_(short a, short b) { return a ? a : b; }

std::string lower(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/* item id by exact name, 0 if missing */
short find_id(const char *name)
{
    const std::string want = lower(name);
    for (const ::item &it : items)
        if (it.raw_name.size() == want.size() && lower(it.raw_name) == want) return static_cast<short>(it.id);
    printf("[blast] no item named '%s'\n", name);
    return 0;
}

/* the id if it really is that item, otherwise looked up by name */
short named(short id, const char *name)
{
    if (id > 0 && id_to_item(static_cast<u_short>(id)).raw_name == name) return id;
    return find_id(name);
}

/* like named(), but only if it can be placed as a plain block (no extra tile data) */
short block_of(short id, const char *name)
{
    const short real = named(id, name);
    if (!real) return 0;
    if (get_type(id_to_item(static_cast<u_short>(real))) != 0)
    {
        printf("[blast] '%s' needs extra tile data - left out\n", name);
        return 0;
    }
    return real;
}

/* like block_of(), but it must be a background, otherwise fallback */
short background_of(short id, const char *name, short fallback)
{
    const short real = block_of(id, name);
    return (real && id_to_item(static_cast<u_short>(real)).type == type::BACKGROUND) ? real : fallback;
}

/* the seed of a block (block id + 1) */
short seed_of(short block_id, const char *block_name)
{
    const short b = named(block_id, block_name);
    if (!b) return 0;
    const short s = static_cast<short>(b + 1);
    return (id_to_item(static_cast<u_short>(s)).type == type::SEED) ? s : 0;
}

heights hills(int base, int lo, int hi, int roughness, int step = 1)
{
    heights h{};
    int y = base;
    for (int x = 0; x < W; ++x)
    {
        if (RandomRange(0, 100) < roughness) y += RandomRange(-step, step + 1);
        y = std::clamp(y, lo, hi);
        h[x] = y;
    }
    return h;
}

heights flat(int y) { heights h{}; h.fill(y); return h; }

void flatten(heights &h, int dx)
{
    for (int x = dx - 2; x <= dx + 2; ++x) if (x >= 0 && x < W) h[x] = h[dx];
}

struct palette
{
    short fill = 2, top = 0, rock = 10, lava = 4, bedrock = 8, bottom = 8, door_bedrock = 8, bg = 14;
    int rock_pct = 5, lava_pct = 37;
};

void reset(::world &w)
{
    w.blocks.assign(W * H, ::block{ 0, 0 });
    w.doors.clear(); w.trees.clear(); w.objects.clear();
    w.signs.clear(); w.displays.clear(); w.random_blocks.clear();
    w.last_object_uid = 0;
}

void ground(::world &w, const heights &h, const palette &p)
{
    for (int x = 0; x < W; ++x)
        for (int y = h[x]; y < H; ++y)
        {
            ::block &b = at(w, x, y);
            b.bg = p.bg;
            if (y >= BEDROCK_ROW) b.fg = (y == H - 1) ? p.bottom : p.bedrock;
            else if (y == h[x] && p.top) b.fg = p.top;
            else if (y > h[x] && y < 50 && p.rock && chance(p.rock_pct)) b.fg = p.rock;
            else if (y > 50 && p.lava && chance(p.lava_pct)) b.fg = p.lava;
            else b.fg = p.fill;
        }
}

/* main door standing on the tile `surface`, with bedrock under it */
void place_door(::world &w, int x, int surface, short bedrock)
{
    const int y = surface - 1;
    at(w, x, y).fg = 6;
    at(w, x, surface).fg = bedrock;
    w.doors.emplace_back(::door("EXIT", "", "", ::pos{ x, y }));
    w.spawn = ::pos{ x, y }.by_32(false);
}

template<typename F>
void decorate(::world &w, const heights &h, int dx, short id, int pct, F ok)
{
    if (!id) return;
    for (int x = 1; x < W - 1; ++x)
    {
        if (std::abs(x - dx) <= 2 || !ok(x)) continue;
        const int y = h[x] - 1;
        if (!inside(x, y) || at(w, x, y).fg != 0) continue;
        if (chance(pct)) at(w, x, y).fg = id;
    }
}
void decorate(::world &w, const heights &h, int dx, short id, int pct) { decorate(w, h, dx, id, pct, [](int) { return true; }); }

void drop(::world &w, short id, short count, int x, int y)
{
    if (!id || !inside(x, y)) return;
    w.objects.emplace_back(::object(static_cast<u_short>(id), static_cast<u_short>(count), ::pos{ x * 32.0f + 8.0f, y * 32.0f + 8.0f }, ++w.last_object_uid));
}

/* a fully grown tree */
void plant(::world &w, int x, int y, short seed)
{
    if (!seed || !inside(x, y)) return;
    const ::item &s = id_to_item(static_cast<u_short>(seed));
    if (s.type != type::SEED) return;
    at(w, x, y).fg = seed;
    w.trees.emplace_back(ticks() - static_cast<u_int>(s.tick), static_cast<u_char>(RandomRange(1, 5)), ::pos{ x, y });
}

void wet(::world &w, int x, int y) { if (inside(x, y)) at(w, x, y).state[3] |= S_WATER; }

/* ------------------------------------------------------------------ worlds */

void make_thermonuclear(::world &w)
{
    reset(w);
    palette p; p.fill = 0; p.rock = 0; p.lava = 0; p.bg = 0;
    ground(w, flat(BEDROCK_ROW), p);
    place_door(w, chance(50) ? 1 : W - 2, 37, 8);
}

void make_desert(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(442, "Sand"), 2);
    p.rock = block_of(332, "Sandstone");
    heights h = hills(37, 34, 39, 25);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short boulder = block_of(944, "Boulder"))
        for (int i = 0; i < 25; ++i) { const int x = RandomRange(0, W), y = RandomRange(40, 50); if (at(w, x, y).fg == p.fill) at(w, x, y).fg = boulder; }
    place_door(w, dx, h[dx], 8);

    const short stone = or_(block_of(332, "Sandstone"), p.fill);
    const short wall = background_of(434, "Sandstone Wall", 0);
    const short egypt = background_of(436, "Wall Like An Egyptian", wall);
    std::vector<int> used{ dx };
    int top_x = -1, top_y = -1;
    for (int size : { 15, 11, 7 })
    {
        int cx = -1;
        for (int tries = 0; tries < 60 && cx < 0; ++tries)
        {
            const int c = RandomRange(size / 2 + 1, W - size / 2 - 1);
            if (std::ranges::all_of(used, [&](int u) { return std::abs(u - c) > size / 2 + 8; })) cx = c;
        }
        if (cx < 0) continue;
        used.push_back(cx);
        int base = H;
        for (int x = cx - size / 2; x <= cx + size / 2; ++x) base = std::min(base, h[x]);
        base -= 1;
        for (int r = 0; r <= size / 2; ++r)
        {
            const int y = base - r;
            for (int x = cx - (size / 2 - r); x <= cx + (size / 2 - r); ++x)
            {
                if (!inside(x, y)) continue;
                ::block &b = at(w, x, y);
                b.fg = stone;
                b.bg = (r > 0 && std::abs(x - cx) < size / 2 - r - 1) ? egypt : wall;
            }
        }
        for (int x = cx - size / 2; x <= cx + size / 2; ++x)
            for (int y = base + 1; y < h[x]; ++y) at(w, x, y).fg = stone;
        if (top_x < 0) { top_x = cx; top_y = base - size / 2 - 1; }
    }
    if (top_x >= 0)
    {
        const int roll = RandomRange(0, 100);
        const short prize = roll < 45 ? named(976, "Sungate") : roll < 90 ? named(970, "Silver Idol")
                          : roll < 95 ? named(12292, "Anubis Pharaoh Mask") : named(12294, "Pharaoh Mask");
        drop(w, prize, 1, top_x, top_y);
    }
    decorate(w, h, dx, block_of(330, "Cactus"), 7);
    decorate(w, h, dx, block_of(974, "Obelisk"), 1);
    decorate(w, h, dx, block_of(972, "Ancient Stone Gate"), 1);
}

void make_jungle(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(1536, "Deep Sand"), 2);
    p.rock = or_(block_of(1538, "Deep Rock"), 10);
    p.rock_pct = 8;
    heights h = hills(37, 34, 40, 30);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short quicksand = block_of(11210, "Quicksand Block"))
        for (int x = 0; x < W; ++x) if (std::abs(x - dx) > 2 && chance(4)) at(w, x, h[x]).fg = quicksand;
    place_door(w, dx, h[dx], 8);

    const short ancient = or_(block_of(4702, "Ancient Block"), p.fill);
    const short wall = background_of(434, "Sandstone Wall", 0);
    int zx = -1;
    for (int tries = 0; tries < 60 && zx < 0; ++tries) { const int c = RandomRange(12, W - 12); if (std::abs(c - dx) > 20) zx = c; }
    if (zx >= 0)
    {
        int base = H;
        for (int x = zx - 10; x <= zx + 10; ++x) base = std::min(base, h[x]);
        base -= 1;
        for (int step = 0; step < 5; ++step)
            for (int r = 0; r < 2; ++r)
            {
                const int y = base - step * 2 - r, half = 10 - step * 2;
                for (int x = zx - half; x <= zx + half; ++x)
                    if (inside(x, y)) { at(w, x, y).fg = ancient; at(w, x, y).bg = wall; }
            }
        for (int x = zx - 10; x <= zx + 10; ++x)
            for (int y = base + 1; y < h[x]; ++y) at(w, x, y).fg = ancient;
        for (int x = zx - 2; x <= zx + 2; ++x)
            for (int y = base - 3; y <= base - 1; ++y) at(w, x, y).fg = 0; // @note hidden chamber
        const int roll = RandomRange(0, 100);
        const short prize = roll < 50 ? named(4778, "Adventurer's Whip") : roll < 80 ? named(4714, "Minecart")
                          : roll < 85 ? named(13766, "Koala Hood") : roll < 90 ? named(13782, "Raccoon Hood")
                          : roll < 95 ? named(10066, "Riding Gorilla") : named(11188, "Wearable Ostrich");
        drop(w, prize, 1, zx, base - 1);
    }

    int sx = -1;
    for (int tries = 0; tries < 60 && sx < 0; ++tries)
    {
        const int c = RandomRange(3, W - 3);
        if (std::abs(c - dx) > 6 && (zx < 0 || std::abs(c - zx) > 14)) sx = c;
    }
    if (sx >= 0)
    {
        const short body = block_of(4784, "Statue Block"), eye = block_of(4786, "Statue Eye"), head = block_of(4788, "Statue Headdress"),
                    nose = block_of(4790, "Statue Nose"), mouth = block_of(4792, "Statue Mouth");
        const int g = std::min({ h[sx - 1], h[sx], h[sx + 1] }) - 1;
        const short layout[5][3] = { { body, head, body }, { eye, body, eye }, { body, nose, body }, { body, mouth, body }, { body, body, body } };
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 3; ++c)
            {
                const int x = sx - 1 + c, y = g - 4 + r;
                if (inside(x, y) && layout[r][c]) at(w, x, y).fg = layout[r][c];
            }
        for (int x = sx - 1; x <= sx + 1; ++x)
            for (int y = g + 1; y < h[x]; ++y) at(w, x, y).fg = body ? body : p.fill;
    }
    decorate(w, h, dx, block_of(4782, "Jungle Fern"), 8);
    decorate(w, h, dx, block_of(854, "Palm Tree"), 4);
    decorate(w, h, dx, block_of(1102, "Sequoia Tree"), 3);
    decorate(w, h, dx, block_of(4798, "Overgrown Vines"), 3);
}

void make_mars(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(1132, "Martian Soil"), 2);
    p.rock = or_(block_of(1134, "Mars Rock"), 10);
    p.rock_pct = 8;
    heights h = hills(37, 32, 41, 35);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short ice = block_of(440, "Ice"))
        for (int i = 0; i < 8; ++i)
        {
            const int cx = RandomRange(0, W), cy = RandomRange(40, 51), r = RandomRange(1, 3);
            for (int x = cx - r; x <= cx + r; ++x)
                for (int y = cy - r; y <= cy + r; ++y)
                    if (inside(x, y) && at(w, x, y).fg == p.fill) at(w, x, y).fg = ice;
        }
    place_door(w, dx, h[dx], 8);
    decorate(w, h, dx, block_of(1138, "Martian Tree"), 5);
}

void make_cave(::world &w)
{
    reset(w);
    const short dirt = or_(block_of(3564, "Cave Dirt"), 2);
    const short bg = background_of(3556, "Dark Cave Background", 14);
    for (int x = 0; x < W; ++x)
        for (int y = 0; y < H; ++y)
        {
            ::block &b = at(w, x, y);
            b.bg = bg;
            b.fg = (y >= BEDROCK_ROW) ? 8 : (y >= 50 && chance(20)) ? 4 : dirt;
        }
    auto carve = [&](int cx, int cy, int r)
    {
        for (int x = cx - r; x <= cx + r; ++x)
            for (int y = cy - r; y <= cy + r; ++y)
                if (inside(x, y) && y >= 1 && y < 50) at(w, x, y).fg = 0;
    };
    for (int worm = 0; worm < 9; ++worm)
    {
        int x = RandomRange(5, 95), y = RandomRange(4, 48);
        for (int step = 0; step < 140; ++step)
        {
            carve(x, y, 1);
            x = std::clamp(x + RandomRange(-1, 2), 2, W - 3);
            if (chance(30)) y = std::clamp(y + RandomRange(-1, 2), 3, 48);
        }
    }
    const int dx = RandomRange(6, W - 6), dy = RandomRange(12, 28);
    for (int x = dx - 4; x <= dx + 4; ++x)
        for (int y = dy - 4; y <= dy; ++y) at(w, x, y).fg = 0;
    for (int x = dx - 4; x <= dx + 4; ++x) if (at(w, x, dy + 1).fg == 0) at(w, x, dy + 1).fg = dirt; // @note floor under the door pocket

    const short stalagmite = block_of(3568, "Stalagmite"), stalactite = block_of(3570, "Stalactite"),
                crystal = block_of(3584, "Aqua Cave Crystal"), column = block_of(3566, "Cave Column"),
                iron = block_of(3608, "Deep Iron"), hoard = block_of(3604, "Golden Treasure Hoard");
    int hoards = 0;
    for (int x = 1; x < W - 1; ++x)
        for (int y = 2; y < 50; ++y)
        {
            ::block &b = at(w, x, y);
            if (b.fg == dirt) { if (iron && y > 20 && chance(3)) b.fg = iron; continue; }
            if (b.fg != 0 || (std::abs(x - dx) <= 4 && y >= dy - 4 && y <= dy)) continue;
            const bool floor = at(w, x, y + 1).fg == dirt, ceiling = at(w, x, y - 1).fg == dirt;
            if (floor && hoard && hoards < 2 && chance(1)) { b.fg = hoard; ++hoards; }
            else if (floor && stalagmite && chance(6)) b.fg = stalagmite;
            else if (floor && crystal && chance(2)) b.fg = crystal;
            else if (floor && column && chance(1)) b.fg = column;
            else if (ceiling && stalactite && chance(6)) b.fg = stalactite;
        }
    for (int tries = 0; hoard && hoards < 2 && tries < 3000; ++tries) // @note deep treasure: always 2 hoards
    {
        const int x = RandomRange(1, W - 1), y = RandomRange(30, 50);
        if (at(w, x, y).fg == 0 && at(w, x, y + 1).fg == dirt && std::abs(x - dx) > 4) { at(w, x, y).fg = hoard; ++hoards; }
    }
    place_door(w, dx, dy + 1, 8);
    drop(w, named(0, "Hand Torch"), 10, dx + 1, dy);
}

void make_beach(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(442, "Sand"), 2);
    p.rock_pct = 3;
    p.bg = background_of(850, "Ocean Rock", 14);
    heights h = hills(37, 35, 38, 20);
    const bool left = chance(50);
    auto ocean = [&](int x) { return left ? x < 40 : x >= 60; };
    for (int x = 0; x < W; ++x)
        if (ocean(x)) { const int d = left ? 40 - x : x - 59; h[x] = std::min(46, 38 + d / 3); }
    const int dx = left ? RandomRange(50, W - 4) : RandomRange(4, 50);
    flatten(h, dx);
    ground(w, h, p);
    for (int x = 0; x < W; ++x)
        if (ocean(x)) for (int y = 37; y < h[x]; ++y) wet(w, x, y);
    if (const short chest = block_of(596, "Treasure Chest"))
        for (int i = RandomRange(1, 4); i > 0; --i)
        {
            const int x = left ? RandomRange(42, W - 1) : RandomRange(1, 58);
            const int y = h[x] + RandomRange(2, 6);
            if (std::abs(x - dx) > 2 && at(w, x, y).fg == p.fill) { at(w, x, y).fg = chest; w.treasure_spots += std::format("{},{};", x, y); } // @note has treasure
        }
    place_door(w, dx, h[dx], 8);
    auto land = [&](int x) { return !ocean(x); };
    decorate(w, h, dx, block_of(854, "Palm Tree"), 6, land);
    decorate(w, h, dx, block_of(848, "Beach Umbrella"), 4, land);
    decorate(w, h, dx, block_of(846, "Seaweed"), 12, ocean);
    decorate(w, h, dx, block_of(832, "Coral"), 6, ocean);
    decorate(w, h, dx, block_of(5038, "Anemone"), 4, ocean);
    decorate(w, h, dx, block_of(8252, "Sea Urchin"), 3, ocean);
}

void make_undersea(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(1536, "Deep Sand"), 2);
    p.rock = or_(block_of(1538, "Deep Rock"), 10);
    p.rock_pct = 8;
    p.bg = background_of(850, "Ocean Rock", 14);
    heights h = hills(40, 36, 44, 30);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    place_door(w, dx, h[dx], 8);
    for (int x = 0; x < W; ++x)
        for (int y = 0; y < h[x]; ++y) wet(w, x, y);

    const short seaweed = block_of(846, "Seaweed"), coral = block_of(832, "Coral");
    for (int x = 1; x < W - 1; ++x)
    {
        if (std::abs(x - dx) <= 2 || !seaweed || !chance(18)) continue;
        for (int y = h[x] - 1, n = RandomRange(1, 4); n > 0 && y > 0 && at(w, x, y).fg == 0; --y, --n) at(w, x, y).fg = seaweed;
    }
    if (coral)
        for (int castle = 0; castle < 3; ++castle)
        {
            const int cx = RandomRange(3, W - 3);
            if (std::abs(cx - dx) < 6) continue;
            for (int x = cx - 2; x <= cx + 2; ++x)
                for (int y = h[x] - 1, n = RandomRange(2, 6); n > 0 && y > 0; --y, --n) at(w, x, y).fg = coral;
        }
    auto on_floor = [&](short id, int count)
    {
        if (!id) return;
        for (int i = 0; i < count; ++i)
        {
            const int x = RandomRange(1, W - 1);
            if (std::abs(x - dx) > 10 && at(w, x, h[x] - 1).fg == 0) at(w, x, h[x] - 1).fg = id;
        }
    };
    on_floor(block_of(1520, "Great White Shark"), RandomRange(0, 3));
    on_floor(block_of(0, "Giant Clam"), RandomRange(1, 3));
    if (chance(20)) on_floor(block_of(0, "Sunken Anchor"), 1);
    if (const short jelly = block_of(0, "Jellyfish"))
        for (int i = RandomRange(1, 4); i > 0; --i)
        {
            const int x = RandomRange(1, W - 1), y = RandomRange(10, std::max(11, h[x] - 5));
            if (std::abs(x - dx) > 4 && at(w, x, y).fg == 0) at(w, x, y).fg = jelly;
        }
}

void make_harvest_moon(::world &w)
{
    reset(w);
    palette p;
    const heights h = flat(37);
    const int dx = RandomRange(4, W - 4);
    ground(w, h, p);
    place_door(w, dx, h[dx], 8);
    std::vector<short> seeds{};
    for (const char *n : { "Autumn Leaf Block", "Pinecone", "Maple Leaf", "Dirt", "Rock", "Cave Background", "Wood Block",
                           "Grass", "Sand", "Sandstone", "Glass Pane", "Bricks", "Wooden Platform", "Lava", "Daisy" })
        if (const short s = seed_of(0, n)) seeds.push_back(s);
    if (seeds.empty()) return;
    for (int x = 0; x < W; ++x)
        if (std::abs(x - dx) > 1) plant(w, x, h[x] - 1, seeds[RandomRange(0, static_cast<int>(seeds.size()))]);
}

void make_bountiful(::world &w)
{
    reset(w);
    palette p;
    heights h = hills(37, 33, 40, 30);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short soil = block_of(8772, "Fertile Soil Block"))
        for (int x = 0; x < W; ++x) if (chance(35)) at(w, x, h[x]).fg = soil;
    place_door(w, dx, h[dx], 8);
    std::vector<short> seeds{};
    for (const auto &[id, name] : { std::pair<short, const char*>{ 8622, "Bountiful Flowering Lattice" }, { 8670, "Bountiful Bamboo Background" },
                                    { 8694, "Bountiful White Doll's Eyes" }, { 8646, "Bountiful Jungle Temple" } })
        if (const short s = seed_of(id, name)) seeds.push_back(s);
    if (seeds.empty()) return;
    for (int i = 0; i < 14; ++i)
    {
        const int x = RandomRange(1, W - 1);
        if (std::abs(x - dx) > 2 && at(w, x, h[x] - 1).fg == 0) plant(w, x, h[x] - 1, seeds[RandomRange(0, static_cast<int>(seeds.size()))]);
    }
}

void make_monochrome(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(7374, "Monochromatic Dirt"), 2);
    p.bg = background_of(7378, "Monochromatic Cave Background", 14);
    p.lava = or_(block_of(7376, "Monochromatic Lava"), 4);
    p.bedrock = p.bottom = p.door_bedrock = or_(block_of(7372, "Monochromatic Bedrock"), 8);
    const heights h = flat(37);
    const int dx = RandomRange(4, W - 4);
    ground(w, h, p);
    if (const short onyx = block_of(7382, "Onyx Block"))
        for (int x = 0; x < W; ++x)
            for (int y = 40; y < BEDROCK_ROW; ++y) if (at(w, x, y).fg == p.fill && RandomRange(0, 1000) < 15) at(w, x, y).fg = onyx;
    place_door(w, dx, h[dx], p.door_bedrock);
}

void make_candyland(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(14898, "Colorful Cake"), 2);
    p.top = block_of(14902, "Colorful Cake Toppings");
    p.rock = 0;
    if (const short slice = block_of(14900, "Colorful Cake Slice"))
    {
        if (id_to_item(static_cast<u_short>(slice)).type == type::BACKGROUND) p.bg = slice;
        else { p.rock = slice; p.rock_pct = 6; }
    }
    p.lava = or_(block_of(14906, "Hot Fudge"), 4);
    p.bedrock = p.door_bedrock = or_(block_of(14908, "Bedrock Candy"), 8);
    p.bottom = or_(block_of(14910, "Data Bedrock Candy"), p.bedrock);
    heights h = hills(37, 34, 39, 25);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    place_door(w, dx, h[dx], p.door_bedrock);
    decorate(w, h, dx, block_of(14904, "Cherry on Top"), 8);
}

void make_fruit_kingdom(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(16194, "Fruit Sand"), 2);
    p.rock = block_of(16200, "Rock Melon");
    p.rock_pct = 6;
    p.lava = 0;
    p.bedrock = p.door_bedrock = or_(block_of(16206, "Bedrock Melon"), 8);
    p.bottom = or_(block_of(16208, "Data Bedrock Melon"), p.bedrock);
    p.bg = background_of(16196, "Fruit Sand Background", 14);
    heights h = hills(37, 34, 39, 25);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short fruity = block_of(16198, "Fruit in Sand"))
        for (int x = 0; x < W; ++x)
            for (int y = h[x] + 1; y < 50; ++y) if (at(w, x, y).fg == p.fill && chance(8)) at(w, x, y).fg = fruity;
    place_door(w, dx, h[dx], p.door_bedrock);
    decorate(w, h, dx, block_of(16204, "Spikey Leaves"), 5);

    std::vector<short> fruits{};
    for (const auto &[id, name] : { std::pair<short, const char*>{ 13512, "Watermelon Block" }, { 13510, "Fresh Watermelon Block" },
                                    { 13516, "Papaya Block" }, { 13514, "Fresh Papaya Block" }, { 13520, "Kiwi Block" }, { 13518, "Fresh Kiwi Block" },
                                    { 13524, "Dragon Fruit Block" }, { 13522, "Fresh Dragon Fruit Block" }, { 2732, "Pineapple Block" }, { 13526, "Fresh Pineapple Block" } })
        if (const short f = block_of(id, name)) fruits.push_back(f);
    if (fruits.empty()) return;
    for (int x = 1; x < W - 1; ++x)
    {
        if (std::abs(x - dx) <= 2 || !chance(10)) continue;
        const short f = fruits[RandomRange(0, static_cast<int>(fruits.size()))];
        for (int y = h[x] - 1, n = RandomRange(1, 4); n > 0 && y > 0 && at(w, x, y).fg == 0; --y, --n) at(w, x, y).fg = f;
    }
}

void make_treasure(::world &w)
{
    reset(w);
    palette p;
    p.fill = or_(block_of(7620, "Frozen Stone Cliffs"), 10);
    p.rock = block_of(7624, "Rune Stone");
    p.rock_pct = 3;
    p.lava = 0;
    p.bg = background_of(7622, "Glacier Background", 14);
    heights h = hills(32, 18, 42, 70, 2);
    const int dx = RandomRange(4, W - 4);
    flatten(h, dx);
    ground(w, h, p);
    if (const short ice = block_of(440, "Ice"))
        for (int x = 0; x < W; ++x) if (std::abs(x - dx) > 2 && (h[x] < 28 || chance(20))) at(w, x, h[x]).fg = ice;
    place_door(w, dx, h[dx], 8);

    if (const short pillar = block_of(7630, "Rune Carved Stone Pillar"))
        for (int i = 0; i < 3; ++i)
        {
            const int x = RandomRange(2, W - 2);
            if (std::abs(x - dx) <= 3) continue;
            for (int y = h[x] - 1, n = RandomRange(2, 5); n > 0 && y > 0 && at(w, x, y).fg == 0; --y, --n) at(w, x, y).fg = pillar;
        }
    decorate(w, h, dx, block_of(2226, "Icicles"), 3);
    decorate(w, h, dx, block_of(7626, "Altar"), 1);
    if (const short hidden = block_of(7628, "Hidden Treasure"))
        for (int i = 0; i < 3; ++i)
        {
            const int x = RandomRange(1, W - 1), y = h[x] + RandomRange(2, 8);
            if (std::abs(x - dx) > 2 && inside(x, y) && at(w, x, y).fg == p.fill) at(w, x, y).fg = hidden;
        }
    for (int pool = 0; pool < 2; ++pool)
    {
        const int x0 = RandomRange(2, W - 8);
        if (std::abs(x0 - dx) < 8) continue;
        for (int x = x0; x < x0 + 5; ++x)
            for (int y = h[x] - 2; y < h[x]; ++y) if (inside(x, y) && at(w, x, y).fg == 0) wet(w, x, y);
    }
}

struct blast_def { short item; int weather; void (*make)(::world&); };
const blast_def blasts[] = {
    { 1402, 0,  make_thermonuclear },
    { 830,  1,  make_beach },
    { 942,  3,  make_desert },
    { 1060, 6,  make_harvest_moon },
    { 1136, 7,  make_mars },
    { 1532, 14, make_undersea },
    { 3562, 0,  make_cave },
    { 4774, 32, make_jungle },
    { 7380, 43, make_monochrome },
    { 7588, 44, make_treasure },
    { 8738, 46, make_bountiful },
    { 14896, 78, make_candyland },
    { 16172, 0,  make_fruit_kingdom },
};

bool world_in_db(const std::string &name)
{
    ::hStmt h{ "SELECT 1 FROM world WHERE name = ? LIMIT 1" };
    MYSQL_BIND param = make_bind_in(name);
    h.bind_param(&param);
    h.execute();
    mysql_stmt_store_result(h.pStmt);
    return mysql_stmt_num_rows(h.pStmt) > 0;
}

int count_of(const ::peer &p, short id)
{
    for (const ::slot &s : p.slots) if (s.id == id) return s.count;
    return 0;
}
} // namespace

/* called when the blast dialog is confirmed */
void blast_create(ENetEvent &event, int id, std::string name)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front()))) name.erase(0, 1);
    for (char &c : name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (name.empty() || name.size() > 24 || !alnum(name))
    {
        tell(event.peer, "`4World names can only use letters and numbers (1-24).``");
        return;
    }
    const auto def = std::ranges::find(blasts, static_cast<short>(id), &blast_def::item);
    if (def == std::end(blasts))
    {
        tell(event.peer, "`4That blast can't create a world on this server yet.``");
        return;
    }
    if (count_of(*pPeer, def->item) < 1) return;
    if (def->item == 830 && pPeer->role < DEVELOPER && count_of(*pPeer, 834) < 100)
    {
        tell(event.peer, "`4The Beach Blast needs `w100 Fireworks`` to detonate!``");
        return;
    }
    if (std::ranges::find(worlds, name, &::world::name) != worlds.end() || world_in_db(name))
    {
        tell(event.peer, "`4That world already exists - pick a new name.``");
        return;
    }

    {
        ::world w{ name }; // @note a brand new world
        def->make(w);
        w.base_weather = def->weather;
    } // @note saved here

    modify_item_inventory(event, ::slot(def->item, -1));
    if (def->item == 830 && pPeer->role < DEVELOPER) modify_item_inventory(event, ::slot(834, -100));
    printf("[blast] %s created %s with %s\n", pPeer->growid.c_str(), name.c_str(), id_to_item(static_cast<u_short>(def->item)).raw_name.c_str());

    if (pPeer->netid != 0) action::quit_to_exit(event, "", true);
    action::join_request(event, "", name);
}


/* called when a block breaks: loot for blast treasure blocks */
void blast_treasure_break(ENetEvent &event, ::world &world, const ::item &item, const ::gamePacket &gamePacket)
{
    const ::pos spot = gamePacket.punch.by_32();
    if (item.type == type::TREASURE || item.id == 596 || item.id == 7628) (void)0; // @note debug line removed

    if (item.id == 596 && world.base_weather == 1) // @note Treasure Chest in a Beach Blast world
    {
        static const std::vector<short> loot = []
        {
            std::vector<short> v{};
            for (const char *n : { "Atomic Fireball", "Barbecue Grill", "Beach Ball", "Beach Blast", "Body Tattoos", "Bubble Machine",
                                   "Frangipani", "Great Ball of Fire", "Greg, The Octopus", "Growmoji Fireworks", "Hydro Cannon",
                                   "Long Surfer Hair", "Oceanic Crown", "Pet Toucan", "Pet Turtle", "Poseidon's Trident", "Riding Flamingo",
                                   "Sandcastle", "Sandtopian", "Sea Monster Floatie", "Seafoam Beard", "Seafoam Hair", "Shark Head",
                                   "Shark Suit", "Sharkzooka", "Short Surfer Hair", "Squirt Gun", "Summer Breeze", "Summer Kite",
                                   "Surfboard", "Swim Fins", "Water Wings", "Watermelon Slice", "White Fury" })
                if (const short id = find_id(n)) v.push_back(id);
            return v;
        }();
        if (loot.empty()) return;
        const short prize = loot[RandomRange(0, static_cast<int>(loot.size()))];
        add_drop(event, ::slot(prize, 1), spot, world);
        tell(event.peer, std::format("`2You found a `w{}`` in the treasure chest!``", id_to_item(static_cast<u_short>(prize)).raw_name));
    }
    else if (item.id == 7628) // @note Hidden Treasure
    {
        static const short fragment = find_id("Map Fragment");
        static const short map = named(7602, "Treasure Map");
        static const short egg = find_id("Stone Egg");

        const short fragments = static_cast<short>(RandomRange(1, 11));
        if (fragment) add_drop(event, ::slot(fragment, fragments), spot, world);
        std::string found = std::format("`w{}`` Map Fragments", fragments);
        if (map && chance(10)) { add_drop(event, ::slot(map, 1), spot, world); found += ", a `wTreasure Map``"; }
        if (egg && chance(5))  { add_drop(event, ::slot(egg, 1), spot, world); found += ", a `wStone Egg``"; }
        tell(event.peer, std::format("`2The Hidden Treasure held {}!``", found));
    }
}

static const std::vector<short> &summer_loot()
{
    static const std::vector<short> loot = []
    {
        std::vector<short> v{};
        for (const char *n : { "Atomic Fireball", "Barbecue Grill", "Beach Ball", "Beach Blast", "Body Tattoos", "Bubble Machine",
                               "Frangipani", "Great Ball of Fire", "Greg, The Octopus", "Growmoji Fireworks", "Hydro Cannon",
                               "Long Surfer Hair", "Oceanic Crown", "Pet Toucan", "Pet Turtle", "Poseidon's Trident", "Riding Flamingo",
                               "Sandcastle", "Sandtopian", "Sea Monster Floatie", "Seafoam Beard", "Seafoam Hair", "Shark Head",
                               "Shark Suit", "Sharkzooka", "Short Surfer Hair", "Squirt Gun", "Summer Breeze", "Summer Kite",
                               "Surfboard", "Swim Fins", "Water Wings", "Watermelon Slice", "White Fury" })
            if (const short id = find_id(n)) v.push_back(id);
        return v;
    }();
    return loot;
}

/* punching a Treasure Chest opens it right away. @return true if it was a chest */
bool blast_chest_punch(ENetEvent &event, ::world &world, ::block &block, const ::gamePacket &gamePacket)
{
    if (block.fg != 596) return false;
    (void)0; // @note debug line removed

    const std::string key = std::format("{},{};", static_cast<int>(gamePacket.punch.x), static_cast<int>(gamePacket.punch.y));
    const std::size_t found = (";" + world.treasure_spots).find(";" + key);
    if (found == std::string::npos) return false; // @note no treasure (placed by a player, or already opened): punch it like a normal block
    world.treasure_spots.erase(found, key.size());

    block.state[2] |= S_TOGGLE; // @note show it as opened
    send_tile_update(event, { .id = block.fg, .punch = gamePacket.punch }, block, world);

    const ::pos spot = gamePacket.punch.by_32();
    const std::vector<short> &loot = summer_loot();
    if (world.base_weather == 1 && !loot.empty()) // @note Beach Blast world
    {
        const short prize = loot[RandomRange(0, static_cast<int>(loot.size()))];
        add_drop(event, ::slot(prize, 1), spot, world);
        tell(event.peer, std::format("`2You found a `w{}`` in the treasure chest!``", id_to_item(static_cast<u_short>(prize)).raw_name));
    }
    else
    {
        const short gems = static_cast<short>(RandomRange(5, 21));
        add_drop(event, ::slot(112, gems), spot, world);
        tell(event.peer, std::format("`2You found `w{}`` gems in the treasure chest!``", gems));
    }
    return true;
}