#pragma once
#ifndef PEER_HPP
#define PEER_HPP

    /* id, count */
    struct slot {
        slot(short _id, short _count) : id(_id), count(_count) {}
        short id{0};
        short count{0}; // @note total amount of that item
    };

    /* x, y */ // @todo add float positions
    struct pos {
        pos(int _x, int _y) : x(_x), y(_y) {}
        int x{0};
        int y{0};

        auto operator<=>(const pos&) const = default;
    };

    struct Billboard {
        short id{0}; // @note the item they're selling
        bool show{};
        bool isBuying{};
        int price{1};
        bool perItem{}; // @note true if world locks per item, false if items per world lock
    };

    struct Friend {
        std::string name{};
        bool ignore{};
        bool block{};
        bool mute{};
    };

    enum role : u_char {
        PLAYER, 
        MODERATOR, 
        DEVELOPER
    };

    enum pstate : int
    {
        S_GHOST =       0x01,
        S_DOUBLE_JUMP = 0x02,
        S_DUCT_TAPE =   0x2000
    };
    
    #include <deque>
    #include <array>
    #if defined(_MSC_VER)
        #undef max
        #undef min
    #endif

    class peer {
    public:
        peer& read(const std::string& name);

        signed netid{ 0 }; // @note peer's netid is world identity. this will be useful for many packet sending
        int user_id{}; // @note unqiue user id.
        std::array<std::string, 2zu> ltoken{}; // @note {growid, password}
        std::string prefix{ "w" }; // @note display name color, default: "w" (White)
        u_char role{};
        std::array<float, 10zu> clothing{}; // @note peer's clothing {id, clothing::}
        u_char punch_effect{}; // @note last equipped clothing that has a effect. supporting 0-255 effects.

        unsigned skin_color{ 2527912447 };

        int state{}; // @note using pstate::

        Billboard billboard{};

        std::array<float, 2zu> pos{}; // @note position {x, y}
        std::array<float, 2zu> rest_pos{}; // @note respawn position {x, y}
        bool facing_left{}; // @note peer is directed towards the left direction

        short slot_size{16}; // @note amount of slots this peer has | were talking total slots not itemed slots, to get itemed slots do slot.size()
        std::vector<slot> slots{{18, 1}, {32, 1}}; // @note an array of each slot. storing {id, count}
        /*
        * @brief set slot::count to nagative value if you want to remove an amount. 
        * @return the remaining amount if exeeds 200. e.g. emplace(slot{0, 201}) returns 1.
        */
        short emplace(slot s);
        std::vector<short> fav{};

        signed gems{0};
        std::array<u_short, 2zu> level{ 1, 0 }; // {level, xp} XP formula credits: https://www.growtopiagame.com/forums/member/553046-kasete
        /*
        * @brief add XP safely, this function also handles level up.
        */
        void add_xp(u_short value);

        std::array<std::string, 6zu> recent_worlds{}; // @note recent worlds, a list of 6 worlds, once it reaches 7 it'll be replaced by the oldest
        std::array<std::string, 200zu> my_worlds{}; // @note first 200 relevant worlds locked by peer.
        
        std::deque<std::chrono::steady_clock::time_point> messages; // @note last 5 que messages sent time, this is used to check for spamming

        std::array<Friend, 25> friends;

        u_short fires_removed{};
        
        ~peer();
    };
    #include <memory>

    extern std::unordered_map<ENetPeer*, std::shared_ptr<peer>> _peer;

    extern ENetHost* server;

    #include <functional>

    enum peer_condition
    {
        PEER_ALL, // @note all peer(s)
        PEER_SAME_WORLD // @note only peer(s) in the same world as ENetEvent::peer
    };

    extern std::vector<ENetPeer*> peers(const std::string &world = "", peer_condition condition = PEER_ALL, std::function<void(ENetPeer&)> fun = [](ENetPeer& peer){});

    void safe_disconnect_peers(int signal);

    class state {
    public:
        int packet_create{ 04 };

        int type{};
        int netid{};
        int uid{}; // @todo understand this better @note so far I think this holds uid value
        int peer_state{};
        float count{}; // @todo understand this better
        int id{}; // @note peer's active hand, so 18 (fist) = punching, 32 (wrench) interacting, ect
        std::array<float, 2zu> pos{}; // @note position 1D {x, y}
        std::array<float, 2zu> speed{}; // @note player movement (velocity(x), gravity(y)), higher gravity = smaller jumps
        int idk{};
        ::pos punch{0,0}; // @note punching/placing position 2D {x, y}
        int idk1{};
    };

    enum packet_pos
    {
        P_INIT,
        P_TYPE =       4zu,
        P_NETID =      P_TYPE*2,
        P_UID =        P_TYPE*3,
        P_PEER_STATE = P_TYPE*4,
        P_COUNT      = P_TYPE*5,
        P_ID         = P_TYPE*6,
        P_POS        = P_TYPE*7, // @note 8 bit
        P_SPEED      = P_TYPE*9, // @note 8 bit
        P_IDK        = P_TYPE*11,
        P_PUNCH      = P_TYPE*12, // @note 8 bit
        P_IDK1       = P_TYPE*14
    };

    extern state get_state(const std::vector<std::byte> &&packet);

    /* put it back into it's original form */
    extern std::vector<std::byte> compress_state(const state &s);

    extern void send_inventory_state(ENetEvent &event);

#endif