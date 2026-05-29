#include "pch.hpp"
#include "Action.hpp"

void on::Action(ENetEvent& event, const std::string_view text) 
{
    std::string_view to_slang = 
        (text == "facepalm") ? "fp" : 
        (text == "shrug") ? "idk" : 
        (text == "foldarms") ? "fold" : 
        (text == "fa") ? "fold" : 
        (text == "stubborn") ? "fold" : text;

    packet::create(*event.peer, true, 0, {
        "OnAction", 
        ('/' + std::string(to_slang)).c_str()
    });
}
