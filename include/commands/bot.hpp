#pragma once

/* a fake player kept purely server-side - no ENet connection */
struct bot
{
    int netid{};
    int user_id{};
    std::string name{};
    std::string world{};
    ::pos pos{};
    std::string country{"se"};
    int state{};                       // @note S_GHOST, S_INVISIBLE etc
    std::array<float, 10ull> clothing{}; // @note same slot order as a peer
    u_int skin{2527912447u};
};

extern std::vector<::bot> bots;

extern void bot_cmd(ENetEvent& event, const std::string_view text);
extern void spawn_bots_for(ENetEvent& event, const std::string &world_name);
