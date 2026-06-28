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
    std::string cdn_host{"ubistatic-a.akamaihd.net"};
    std::string cdn_path{"0098/0210520267/cache/"};
};
extern ::server_data gServer_data;

extern ::server_data init_server_data();
