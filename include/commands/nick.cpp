#include "pch.hpp"
#include "onVariant/NameChanged.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/CountryState.hpp"
#include "database/world.hpp"
#include "nick.hpp"

extern std::string display_name_for(const ::peer &p, const ::world &w); // @note commands/lockaccess.cpp
extern std::string title_name(const ::peer &p, const std::string &base);  // @note commands/legendary.cpp

/* the name to show: real name or /nick, with titles (Dr., of Legend, Party colours) */
static std::string shown_name(const ::peer &p)
{
    if (p.netid != 0)
        if (auto w = std::ranges::find(worlds, p.recent_worlds.back(), &::world::name); w != worlds.end())
            return display_name_for(p, *w);
    if (!p.nickname.empty()) return title_name(p, std::format("`w{}``", p.nickname));
    return title_name(p, p.role >= DEVELOPER ? std::format("`6@{}``", p.growid) : std::format("`w{}``", p.growid));
}

/* new name + title flags to you and everyone in the world (the Legendary look needs both) */
static void send_name(ENetEvent &event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    on::NameChanged(event);
    on::CountryState(event);
    if (pPeer->netid == 0) return;
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p)
    {
        ::peer *o = static_cast<::peer*>(p.data);
        if (!o || o->user_id == pPeer->user_id) return;
        send_varlist(&p, { "OnNameChanged", pPeer->display_growid }, pPeer->netid);
        on::CountryStateOf(p, *pPeer);
    });
}

void nick(ENetEvent& event, const std::string_view text)
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

    if (arg.empty())
    {
        pPeer->nickname.clear();
        pPeer->display_growid = shown_name(*pPeer);
        send_name(event);
        on::ConsoleMessage(event.peer, "`2Name reset.``");
        return;
    }

    if (arg.length() > 24)
    {
        on::ConsoleMessage(event.peer, "`4That name is too long.``");
        return;
    }

    pPeer->nickname = arg;
    pPeer->display_growid = shown_name(*pPeer);
    send_name(event);
    on::ConsoleMessage(event.peer, std::format("`2You are now `w{}``. Use `$/nick`` alone to reset.``", arg));
}