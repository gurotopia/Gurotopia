#include "pch.hpp"
#include "network/packet.hpp"
#include "RequestWorldSelectMenu.hpp"

void OnRequestWorldSelectMenu(ENetEvent event) 
{
    auto &peer = _peer[event.peer];
    auto section = [&peer](const auto& range, const char* color) 
    {
        std::string result;
        for (const auto& name : range) 
            if (not name.empty()) 
            {
                auto it = worlds.find(name);
                result += (it != worlds.end()) ? 
                    std::format("add_floater|{}|{}|0.5|{}\n", name, it->second.visitors, color) :
                    std::format("add_floater|{}|0|0.5|{}\n", name, color);
            }
        return result;
    };
    gt_packet(*event.peer, false, 0, {
        "OnRequestWorldSelectMenu", 
            std::format(
                "add_filter|\n"
                "add_heading|Top Worlds<ROW2>|\n{}"
                "add_heading|My Worlds<CR>|\n{}"
                "add_heading|Recently Visited Worlds<CR>|\n{}",
            "add_floater|wotd_world|\u013B WOTD|0|0.5|3529161471\n", // @todo find the top world(s) in 'worlds' map
            section(peer->my_worlds, "2147418367"), 
            section(peer->recent_worlds, "3417414143")
        ).c_str(), 
        0
    });
    gt_packet(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Where would you like to go? (`w{}`` online)", peers(event).size()).c_str()
    });
}