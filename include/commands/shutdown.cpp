#include "pch.hpp"
#include <csignal>
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "shutdown.hpp"

static std::time_t shutdown_at = 0;  // @note 0 = no shutdown planned
static long long last_announced = -1;

static void announce(const std::string &text, bool overlay)
{
    peers("", peer_condition::PEER_ALL, [&](ENetPeer &p)
    {
        const ::peer *t = static_cast<::peer*>(p.data);
        if (!t || t->growid.empty()) return;
        on::ConsoleMessage(&p, text);
        if (overlay) send_varlist(&p, { "OnTextOverlay", text });
    });
}

static std::string readable(long long secs)
{
    if (secs >= 120) return std::format("{} minutes", (secs + 30) / 60);
    if (secs == 60) return "1 minute";
    return std::format("{} second{}", secs, secs == 1 ? "" : "s");
}

/* /shutdown [seconds]   countdown (default 60) then saves everything and stops the server
 * /shutdown cancel      stops the countdown */
void shutdown_cmd(ENetEvent &event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();
    while (!arg.empty() && std::isspace(static_cast<unsigned char>(arg.back()))) arg.pop_back();

    if (arg == "cancel")
    {
        if (shutdown_at == 0) { on::ConsoleMessage(event.peer, "`4No shutdown is planned.``"); return; }
        shutdown_at = 0;
        last_announced = -1;
        announce("`2** The shutdown was cancelled **``", true);
        return;
    }

    long long secs = 60;
    if (!arg.empty())
    {
        if (!std::ranges::all_of(arg, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }) || arg.size() > 5)
        {
            on::ConsoleMessage(event.peer, "`4Usage: /shutdown [seconds]`` or `w/shutdown cancel``");
            return;
        }
        secs = std::clamp(std::stoll(arg), 5LL, 3600LL);
    }

    shutdown_at = std::time(nullptr) + secs;
    last_announced = secs;
    printf("[shutdown] %s started a countdown: %lld seconds\n", pPeer->growid.c_str(), secs);
    announce(std::format("`4** Server shutting down in {}! ** `oPlease finish up.``", readable(secs)), true);
}

void shutdown_tick(std::time_t now)
{
    if (shutdown_at == 0) return;
    const long long left = static_cast<long long>(shutdown_at) - static_cast<long long>(now);

    if (left <= 0)
    {
        announce("`4** Server shutting down now ** `oSaving worlds, back shortly.``", true);
        for (::world &w : worlds) w.save();
        shutdown_at = 0;
        printf("[shutdown] countdown finished\n");
        raise(SIGINT); // @note same path as pressing Ctrl+C
        return;
    }

    static constexpr long long marks[]{ 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 4, 3, 2, 1 };
    if (std::ranges::find(marks, left) == std::end(marks) || left == last_announced) return;
    last_announced = left;
    announce(std::format("`4** Server shutting down in {}! **``", readable(left)), left <= 10);
}
