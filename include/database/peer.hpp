#pragma once

#include <array>
#include <cmath>
#include <deque>
#include <functional>

enum bgra : u_int
{
    BLUE  = 0xff000000,
    GREEN = 0x00ff0000,
    RED   = 0x0000ff00,
    ALPHA = 0x000000ff
};

/* id, count */
struct slot {
    slot(short _id, short _count) : id(_id), count(_count) {}
    short id{0};
    short count{0}; // @note total amount of that item
};

/* x, y */
struct pos {
    pos() = default;
    pos(float _x, float _y) : x(_x), y(_y) {}
    pos(int _x, int _y)     : x(_x), y(_y) {}

    float x, y;

    /* use this for pixeled position - i did my best t-t */
    pos by_32(bool pixel = false) const { 
        return (pixel) ? 
            pos{std::floor/*<auto>*/(x/32.0f), std::floor/*<auto>*/(y/32.0f)} : 
            pos{x*32.0f, y*32.0f}; 
    }

    int x_int() const { return std::floor<int>(x); }
    int y_int() const { return std::floor<int>(y); }

    u_int x_u_int() const { return std::floor<u_int>(x); }
    u_int y_u_int() const { return std::floor<u_int>(y); }

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
    S_GHOST       = 0x00000001,
    S_DOUBLE_JUMP = 0x00000002,
    S_DUCT_TAPE   = 0x00002000
};

class peer {
public:
    bool exists(const std::string& growid);

    template<typename T>
    void mysql_insert(const std::string& column, const T& value);

    template<typename T>
    void mysql_update(const std::string& column, const T& value);

    template<typename T>
    T    mysql_select(const std::string &column, const std::string &arg = "");
    void mysql_select_all();

    int user_id{}; // @note unqiue user id.
    std::string growid{""}, password{""};
    std::time_t created_at{}; // @note when inserted in SQL (account age)
    u_char role{};
    std::array<float, 10ull> clothing{}; // @note peer's clothing {id, clothing::}
    u_char punch_effect{}; // @note last equipped clothing that has a effect. supporting 0-255 effects.

    int netid{}; // @note peer's netid is world identity. this will be useful for many packet sending
    std::string prefix{ 'w'  }; // @note display name color, default: "w" (White)
    std::string country{};

    u_int skin_color{ 2527912447 };
    u_int hair_color = bgra::GREEN | bgra::GREEN | bgra::RED | bgra::ALPHA; // @note value declines for specfic hair colors

    int state{}; // @note using pstate::

    ::Billboard billboard{};

    ::pos pos{}; // @note position 1D {x, y}
    ::pos rest_pos{}; // @note respawn position {x, y}
    bool facing_left{}; // @note peer is directed towards the left direction
    short pain_hp{ 10 };

    short slot_size{16}; // @note amount of slots this peer has | were talking total slots not itemed slots, to get itemed slots do slot.size()
    std::vector<slot> slots{}; // @note an array of each slot. storing {id, count}
    /*
    * @brief set slot::count to nagative value if you want to remove an amount. 
    * @return the remaining amount if exeeds 200. e.g. emplace(slot{0, 201}) returns 1.
    */
    u_short emplace(::slot slot);
    std::vector<short> fav{};

    signed gems{0};
    std::array<u_short, 2ull> level{ 1, 0 }; // {level, xp} XP formula credits: https://www.growtopiagame.com/forums/member/553046-kasete
    /*
    * @brief add XP safely, this function also handles level up.
    */
    void add_xp(ENetEvent &event, u_short value);

    /*
    * @brief recalculate transient effects based on clothing.
    */
    void update_effects();

    std::array<std::string, 6ull> recent_worlds{}; // @note recent worlds, a list of 6 worlds, once it reaches 7 it'll be replaced by the oldest
    std::array<std::string, 200ull> my_worlds{}; // @note first 200 relevant worlds locked by peer.
    
    std::deque<std::chrono::steady_clock::time_point> messages; // @note last 5 que messages sent time, this is used to check for spamming

    std::array<Friend, 25> friends;

    u_short fires_removed{};
    u_short gbc_pity{}; // @note GBC pity; for each 100 will receive super GBC
};

extern ENetHost* host;

enum peer_condition
{
    PEER_ALL, // @note all peer(s)
    PEER_SAME_WORLD // @note only peer(s) in the same world as ENetEvent::peer
};

extern std::vector<ENetPeer*> peers(const std::string &world = "", peer_condition condition = PEER_ALL, std::function<void(ENetPeer&)> fun = [](ENetPeer& peer){});

extern void safe_disconnect_peers(int signal);

enum peer_state : int
{
    S_UPDATE          = 0x04,
    S_EXTENDED        = 0x08,
    S_MOVE_LEFT       = 0x10,
    S_MOVE_RIGHT      = 0x20,
    S_LAVA_HIT        = 0x40,
    S_JUMP            = 0x80,
    S_ACTIVATE_OBJECT = 0x4000
};

class state {
public:
    int packet_create{ 04 }; // @note NET_MESSAGE_GAME_PACKET

    int type{};
    int netid{};
    int uid{}; // @todo understand this better @note so far I think this holds uid value
    int peer_state{};
    float count{}; // @todo understand this better
    int id{}; // @note peer's active hand, so 18 (fist) = punching, 32 (wrench) interacting, ect
    ::pos pos{}; // @note position 1D {x, y}
    ::pos speed{}; // @note player movement (velocity(x), gravity(y)), higher gravity = smaller jumps
    int idk{};
    ::pos punch{}; // @note punching/placing position 2D {x, y}
    u_int size{};
};

enum packet_pos : std::size_t
{
    P_INIT,
    P_TYPE       = 4ull,
    P_NETID      = P_TYPE*2ull,
    P_UID        = P_TYPE*3ull,
    P_PEER_STATE = P_TYPE*4ull,
    P_COUNT      = P_TYPE*5ull,
    P_ID         = P_TYPE*6ull,
    P_POS        = P_TYPE*7ull, // @note 8 bit
    P_SPEED      = P_TYPE*9ull, // @note 8 bit
    P_IDK        = P_TYPE*11ull,
    P_PUNCH      = P_TYPE*12ull, // @note 8 bit
    P_SIZE       = P_TYPE*14ull
};

extern state get_state(const std::vector<u_char> &&packet);

/* put it back into it's original form */
extern std::vector<u_char> compress_state(const state &state);

extern void send_inventory_state(ENetEvent &event);
