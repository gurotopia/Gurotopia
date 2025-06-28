#pragma once
#ifndef HTTPS_HPP
#define HTTPS_HPP

#include <unordered_map>
#include <chrono>

namespace https
{
    extern void listener(std::string ip, short enet_port);

    class client
    {
    public:
        std::chrono::steady_clock::time_point last_connect{};
    };
    extern std::unordered_map<const char*, client> clients;
}

#endif