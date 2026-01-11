#pragma once
#ifndef WORLD_HPP
#define WORLD_HPP

    enum wstate3 : u_char
    {
        S_RIGHT =  0x00,
        S_LOCKED = 0x10,
        S_LEFT =   0x20,
        S_TOGGLE = 0x40, // @note including mailbox which just toggles the visuals
        S_PUBLIC = 0x80
    };

    /* locks that only occupy a number of tiles, and not the whole world. */
    #define is_tile_lock(id) (id == 202/*small*/ || id == 204/*big*/ || id == 206/*huge*/ || id == 4994/*builder*/)

    enum wstate4 : u_char
    {
        S_WATER =    0x04,
        S_GLUE =     0x08,
        S_FIRE =     0x10,
        /* paint buckets */
        S_RED =      0x20,
        S_GREEN =    0x40,
        S_YELLOW =   S_RED | S_GREEN,
        S_BLUE =     0x80,
        S_AQUA =     S_GREEN | S_BLUE,
        S_PURPLE =   S_RED | S_BLUE,
        S_CHARCOAL = S_RED | S_GREEN | S_BLUE,
        S_VANISH =   S_RED | S_YELLOW | S_GREEN | S_AQUA | S_BLUE | S_PURPLE | S_CHARCOAL
    };

    struct block 
    {
        block(
            short _fg = 0, short _bg = 0, 
            std::chrono::steady_clock::time_point _tick = std::chrono::steady_clock::time_point(),
            std::string _label = "", u_char _state3 = 0, u_char _state4 = 0
        ) : fg(_fg), bg(_bg), tick(_tick), label(_label), state3(_state3), state4(_state4) {}
        short fg{0}, bg{0};
        
        std::chrono::steady_clock::time_point tick{}; // @note record a point in time for the tile e.g. tree growth, providers, ect.
        std::string label{}; // @note sign/door label

        u_char state3{}; // @note direction; visuals | uses wstate3::
        u_char state4{}; // @note water; glue; fire; paint | uses wstate4::

        std::array<int, 2zu> hits{0, 0}; // @note fg, bg
    };
    #define cord(x,y) (y * 100 + x)

    struct door 
    {
        door(std::string _dest, std::string _id, std::string _password, ::pos _pos) : 
            dest(_dest), id(_id), password(_password), pos(_pos) {}

        std::string dest{};
        std::string id{};
        std::string password{};
        ::pos pos;
    };

    struct ifloat 
    {
        ifloat(short _id, short _count, std::array<float, 2zu> _pos) : id(_id), count(_count), pos(_pos) {}
        short id{0};
        short count{0};
        std::array<float, 2zu> pos;
    };

    class world 
    {
    public:
        world(const std::string& name = "");
        std::string name{};

        int owner{ 00 }; // @note owner of world using peer's user id.
        std::array<int, 6zu> admin{}; // @note admins (by user id). excluding owner. (6 is a experimental amount, if increase update me if any issue occur -leeendl)
        bool _public{}; // @note checks if world is public to break/place

        u_char visitors{0}; // @note the current number of peers in a world, excluding invisable peers

        std::vector<::block> blocks; // @note all blocks, size of 1D meaning (6000) instead of 2D (100, 60)
        std::vector<::door> doors;
        int ifloat_uid{0}; // @note floating item UID
        std::unordered_map<int, ifloat> ifloats{}; // @note (i)tem floating
        ~world();
    };
    extern std::unordered_map<std::string, world> worlds;

    extern void send_data(ENetPeer &peer, const std::vector<std::byte> &&data);

    extern void state_visuals(ENetPeer &peer, state &&s);

    extern void tile_apply_damage(ENetEvent& event, state s, block& b);

    extern void modify_item_inventory(ENetEvent& event, ::slot slot);

    extern int item_change_object(ENetEvent& event, ::slot slot, const std::array<float, 2zu>& pos, signed uid = 0);

    extern void tile_update(ENetEvent &event, state s, block &b, world& w);

    extern void remove_fire(ENetEvent &event, state state, block &block, world& w);

    void generate_world(world &world, const std::string& name);

    bool door_mover(world &world, const ::pos &pos);

    namespace blast
    {
        void thermonuclear(world &world, const std::string& name);
    }

#endif