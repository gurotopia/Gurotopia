#pragma once
#ifndef QUIT_TO_EXIT_HPP
#define QUIT_TO_EXIT_HPP

    namespace action
    { 
        extern void quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection);
    }

#endif