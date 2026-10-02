#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "uptime.hpp"

std::time_t gServer_start_time = std::time(nullptr);

/* /uptime -> how long the server has been running */
void uptime(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const std::time_t now = std::time(nullptr);
    const long long seconds = static_cast<long long>(now - gServer_start_time);

    const long long days = seconds / 86400;
    const long long hours = (seconds % 86400) / 3600;
    const long long minutes = (seconds % 3600) / 60;
    const long long secs = seconds % 60;

    std::string out{};
    if (days > 0)  out += std::format("`w{}``d ", days);
    if (hours > 0) out += std::format("`w{}``h ", hours);
    if (minutes > 0) out += std::format("`w{}``m ", minutes);
    out += std::format("`w{}``s", secs);

    on::ConsoleMessage(event.peer, std::format("`oServer has been up for {}.", out));
}
