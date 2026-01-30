#include "pch.hpp"
#include "action/wrench.hpp"
#include "edit.hpp"

void edit(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("edit ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /edit `w{player name}``" });
        return;
    }
    std::string name{ text.substr(sizeof("edit ")-1) };

    u_char is_online{};
    std::string fmt = 
        "set_default_color|`o\n"
        "add_label|big|Username: `w{0}`` (`s{1}``)|left\n"
        "embed_data|name|{0}\n"
        "add_spacer|small|\n"
        "add_text_input|role|Role: |{2}|3|\n"
        "add_text_input|level|Level: |{3}|3|\n"
        "add_text_input|gems|Gems: |{4}|10|\n"
        "add_spacer|small|\n"
        "embed_data|status|{5}\n"
        "end_dialog|peer_edit|Close|`2Edit|\n";

    peers("", PEER_ALL, [&event, name, &is_online, fmt](ENetPeer& p) 
    {
        ::peer *_p = static_cast<::peer*>(p.data);
        if (_p->ltoken[0] == name)
        {
            is_online = 1;
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::vformat(fmt, std::make_format_args(name, "`2Online", _p->role, _p->level.front(), _p->gems, is_online)).c_str()
            });
            return;
        }
    });
    if (!is_online)
    {
        if (!::peer().exists(name))
        {
            packet::create(*event.peer, false, 0, { "OnConsoleMessage", "`4player doesn't exist``" });
            return;
        }
        peer &offline = ::peer().read(name);

        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::vformat(fmt, std::make_format_args(name, "`sOffline", offline.role, offline.level.front(), offline.gems, is_online)).c_str()
        });
    }
}
