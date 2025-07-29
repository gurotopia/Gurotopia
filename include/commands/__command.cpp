#include "pch.hpp"
#include "on/Action.hpp"
#include "find.hpp"
#include "warp.hpp"
#include "edit.hpp"
#include "punch.hpp"
#include "skin.hpp"
#include "sb.hpp"
#include "who.hpp"
#include "me.hpp"
#include "weather.hpp"
#include "__command.hpp"

std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool
{
    {"help", [](ENetEvent& event, const std::string_view text) 
    {
        packet::action(*event.peer, "log", "msg|>> Commands: /find /warp {world} /edit {player} /punch {id} /skin {id} /sb {msg} /who /me {msg} /weather {id} /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy");
    }},
    {"find", &find},
    {"warp", &warp},
    {"edit", &edit},
    {"punch", &punch},
    {"skin", &skin},
    {"sb", &sb},
    {"who", &who},
    {"me", &me},
    {"weather", &weather},
    {"wave", &on::Action}, {"dance", &on::Action}, {"love", &on::Action}, {"sleep", &on::Action}, {"facepalm", &on::Action}, {"fp", &on::Action}, 
    {"smh", &on::Action}, {"yes", &on::Action}, {"no", &on::Action}, {"omg", &on::Action}, {"idk", &on::Action}, {"shrug", &on::Action}, 
    {"furious", &on::Action}, {"rolleyes", &on::Action}, {"foldarms", &on::Action}, {"fa", &on::Action}, {"stubborn", &on::Action}, {"fold", &on::Action}, 
    {"dab", &on::Action}, {"sassy", &on::Action}, {"dance2", &on::Action}, {"march", &on::Action}, {"grumpy", &on::Action}, {"shy", &on::Action}
};