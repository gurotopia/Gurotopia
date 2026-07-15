#include "pch.hpp"
#include "automate/holiday.hpp"
#include "on/ConsoleMessage.hpp"

#include "time.hpp"

void command::time(ENetEvent& event, const std::string_view text)
{
    /* @todo cleanup this... */
    std::tm time = localtime();
    std::vector<std::string> month = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

    on::ConsoleMessage(event.peer,
        std::format("`2Growtopia Time (EDT/UTC-5): {} {}{}, {:02d}:{:02d}.", /* @todo remove hardcoded UTC zone */
            month[time.tm_mon], time.tm_mday,
            (time.tm_mday >= 11 && time.tm_mday <= 13) ? "th" :
            (time.tm_mday % 10 == 1) ? "st" :
            (time.tm_mday % 10 == 2) ? "nd" :
            (time.tm_mday % 10 == 3) ? "rd" : "th", time.tm_hour, time.tm_min
        )
    );
}