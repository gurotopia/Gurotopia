#pragma once
#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

    namespace action
    { 
        extern void protocol(ENetEvent& event, const std::string& header);
    }

#endif