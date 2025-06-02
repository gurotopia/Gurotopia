#include "database/peer.hpp"
#include "network/packet.hpp"
#include "RequestWorldSelectMenu.hpp"

void OnRequestWorldSelectMenu(ENetEvent event) 
{
    auto section = [](const auto& range, const char* color) 
    {
        std::string result;
        for (const auto& name : range)
            if (not name.empty()) // @note some may be stored empty but still an object. e.g. std::array
                result += std::format("\nadd_floater|{}|0|0.5|{}", name, color);
        return result;
    };
    gt_packet(*event.peer, false, 0, {
        "OnRequestWorldSelectMenu", 
        std::format("add_filter|\nadd_heading|Top Worlds<ROW2>|{}\nadd_heading|Recently Visited Worlds<CR>|{}\n",
        "\nadd_floater|wotd_world|\u013B WOTD|0|0.5|3529161471", section(_peer[event.peer]->recent_worlds, "3417414143")).c_str(), 
        0
    });
    gt_packet(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Where would you like to go? (`w{}`` online)", peers().size()).c_str()
    });
}