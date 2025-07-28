#include "pch.hpp"
#include "action/wrench.hpp"
#include "edit.hpp"

void edit(ENetEvent& event, const std::string_view text)
{
    std::string name{ text.substr(sizeof("edit ")-1) };
    bool was_online{};
    peers(event, PEER_ALL, [&event, name, &was_online](ENetPeer& p) 
    {
        auto &peer = _peer[&p];
        if (peer->ltoken[0] == name)
        {
            was_online = true;
            packet::create(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format(
                    "set_default_color|`o\n"
                    "add_label|big|Username: `w{0}`` (`2Online``)|left\n"
                    "embed_data|name|{0}\n"
                    "add_spacer|small|\n"
                    "add_text_input|role|Role: |{1}|3|\n"
                    "add_text_input|level|Level: |{2}|3|\n"
                    "add_text_input|gems|Gems: |{3}|10|\n"
                    "add_spacer|small|\n"
                    "embed_data|status|1\n"
                    "end_dialog|peer_edit|Close|`2Edit|\n",
                    name, peer->role, peer->level.front(), peer->gems
                ).c_str()
            });
            return;
        }
    });
    if (!was_online)
    {
        peer &offline = ::peer().read(name);

        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label|big|Username: `w{0}`` (`sOffline``)|left\n"
                "embed_data|name|{0}\n"
                "add_spacer|small|\n"
                "add_text_input|role|Role: |{1}|3|\n"
                "add_text_input|level|Level: |{2}|3|\n"
                "add_text_input|gems|Gems: |{3}|10|\n"
                "add_spacer|small|\n"
                "embed_data|status|0\n"
                "end_dialog|peer_edit|Close|`2Edit|\n",
                name, offline.role, offline.level.front(), offline.gems
            ).c_str()
        });
    }
}