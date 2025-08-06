#pragma once
#ifndef HTTPS_HPP
#define HTTPS_HPP

#include <unordered_map>
#include <chrono>
#include "server_data.hpp" // @todo make this independent

namespace https
{
    extern void listener(_server_data server_data);
}

#endif