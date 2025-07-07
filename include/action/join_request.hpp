#pragma once
#ifndef JOIN_REQUEST_HPP
#define JOIN_REQUEST_HPP

    namespace action
    { 
        extern void join_request(ENetEvent& event, const std::string& header, const std::string_view world_name);
    }

#endif