#pragma once

class server_data 
{
public:
    std::string server{"127.0.0.1"};
    u_short port{17091};
    u_char type{1};
    u_char type2{1};
    std::string maint{"Server under maintenance. Please try again later."};
    std::string loginurl{"login-gurotopia.vercel.app"};
    std::string meta{"gurotopia"};

    void init();
};
extern ::server_data gServer_data;

/* the address a player got at login, reused when the game is told where to reconnect */
extern void remember_address(const std::string &client_ip, const std::string &server_ip);
extern std::string server_for_peer(const ENetPeer &p);
