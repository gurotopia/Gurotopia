#include "pch.hpp"
#include "socialportal.hpp"

void socialportal(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    /* buttonClicked */
    if (pipes[5zu] == "showfriend")
    {
        /* temp data */
        u_char __online{};

        // @todo i will improve!! cause i hate this so much t-t
        for (Friend &f : peer->friends)
            peers(event, PEER_ALL, [&f, &__online](ENetPeer& peer){
                auto &p = _peer[&peer];
                if (p->ltoken[0] == f.name)
                {
                    ++__online;
                }
            });

        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|0 of {} `wFriends Online``|left|1366|\n"
                "add_spacer|small|\n"
                "add_textbox|`oNone of your friends are currently online.``|left|\n"
                "add_spacer|small|\n"
                "add_spacer|small|\n"
                "add_button|friend_all|Show offline and ignored too|noflags|0|0|\n"
                "add_button|all_friends|Edit Friends|noflags|0|0|\n"
                "add_button|friends_options|Friend Options|noflags|0|0|\n"
                "add_button|back|Back|noflags|0|0|\n"
                "add_button||Close|noflags|0|0|\n"
                "end_dialog|friends|||\n"
                "add_quick_exit|\n",
                __online
            ).c_str()
        });
    }
}