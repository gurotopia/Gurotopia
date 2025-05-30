#ifndef WORLD_HPP
#define WORLD_HPP

    /* fg, bg, hits */
    class block 
    {
        public:
        short fg{0}, bg{0};
        std::string label{""}; // @note sign/door label
        std::array<int, 2ull> hits{0, 0}; // @note fg, bg
    };

    /* uid, id, count, pos*/
    class ifloat 
    {
        public:
        std::size_t uid{0};
        short id{0};
        short count{0};
        std::array<float, 2ull> pos;
    };

    #include "nlohmann/json.hpp" // @note https://github.com/nlohmann/json

    class world 
    {
    public:
        world& read(std::string name);
        std::string name{};
        int owner{ 00 }; // @note owner of world using peer's user id.
        std::array<int, 6ull> admin{}; // @note admins (by user id). excluding owner. (6 is a experimental amount, if increase update me if any issue occur -leeendl)
        short visitors{0}; // -> stack object
        std::vector<block> blocks; /* all blocks, size of 1D meaning (6000) instead of (100, 60) */
        std::size_t ifloat_uid{0};
        std::vector<ifloat> ifloats{}; /* (i)tem floating */
        ~world();
    };
    extern std::unordered_map<std::string, world> worlds;

    extern void send_data(ENetPeer& peer, const std::vector<std::byte>& data);

    extern void state_visuals(ENetEvent& event, state s);

    extern void block_punched(ENetEvent& event, state s, block& b);

    extern void drop_visuals(ENetEvent& event, const std::array<short, 2ull>& im, const std::array<float, 2ull>& pos, signed uid = 0);

    extern void clothing_visuals(ENetEvent &event);

    extern void tile_update(ENetEvent &event, state s, block &b, world& w);

#endif