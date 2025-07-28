#include "pch.hpp"
#include "action/wrench.hpp"
#include "edit.hpp"

void edit(ENetEvent& event, const std::string_view text)
{
    std::string name{ text.substr(sizeof("edit ")-1) };
    bool was_online{};
    std::string fmt = 
        "set_default_color|`o\n"
        "add_label|big|Username: `w{0}`` (`s{1}``)|left\n"
        "embed_data|name|{0}\n"
        "add_spacer|small|\n"
        "add_text_input|role|Role: |{2}|3|\n"
        "add_text_input|level|Level: |{3}|3|\n"
        "add_text_input|gems|Gems: |{4}|10|\n"
        "add_spacer|small|\n"
        "embed_data|status|0\n"
        "end_dialog|peer_edit|Close|`2Edit|\n";

    peers(event, PEER_ALL, [&event, name, &was_online, fmt](ENetPeer& p) 
    {
        auto &peer = _peer[&p];
        if (_peer[&p]->ltoken[0] == name)
        {
            auto &peer = _peer[&p];
            was_online = true;
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::vformat(fmt, std::make_format_args(name, "`2Online", peer->role, peer->level.front(), peer->gems)).c_str()
            });
            return;
        }
    });
    if (!was_online)
    {
        peer &offline = ::peer().read(name);

        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::vformat(fmt, std::make_format_args(name, "`sOffline", offline.role, offline.level.front(), offline.gems)).c_str()
        });
    }
}