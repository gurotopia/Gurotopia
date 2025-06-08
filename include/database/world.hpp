#ifndef WORLD_HPP
#define WORLD_HPP

    /* fg, bg, hits */
    class block 
    {
    public:
        short fg{0}, bg{0};
        std::string label{""}; // @note sign/door label
        std::array<int, 2zu> hits{0, 0}; // @note fg, bg
    };
    #define cord(x,y) (y * 100 + x)

    /* uid, id, count, pos*/
    class ifloat 
    {
    public:
        std::size_t uid{0zu};
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
        short visitors{0}; // @note the current number of peers in a world, excluding invisable peers
        std::vector<block> blocks; // @note all blocks, size of 1D meaning (6000) instead of 2D (100, 60)
        std::size_t ifloat_uid{0zu}; // @note floating item UID
        std::vector<ifloat> ifloats{}; // @note (i)tem floating
        ~world();
    };
    extern std::unordered_map<std::string, world> worlds;

    extern void send_data(ENetPeer& peer, const std::vector<std::byte>& data);

    extern void state_visuals(ENetEvent& event, state s);

    extern void block_punched(ENetEvent& event, state s, block& b);

    extern void drop_visuals(ENetEvent& event, const std::array<short, 2zu>& im, const std::array<float, 2zu>& pos, signed uid = 0);

    extern void clothing_visuals(ENetEvent &event);

    extern void tile_update(ENetEvent &event, state s, block &b, world& w);

#endif