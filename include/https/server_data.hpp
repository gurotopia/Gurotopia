#pragma once
#ifndef SERVER_DATA_HPP
#define SERVER_DATA_HPP

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
    };
    extern ::server_data g_server_data; // @note global

    extern ::server_data init_server_data();

#endif