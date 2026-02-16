#pragma once
#ifndef BUY_HPP
#define BUY_HPP

    namespace action
    { 
        extern void buy(ENetEvent& event, const std::string& header, const std::string_view selection);
    }

#endif