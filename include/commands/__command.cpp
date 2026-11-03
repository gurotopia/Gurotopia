#include "pch.hpp"

#include "onVariant/Action.hpp"

#include "msg.hpp"
#include "sb.hpp"
#include "time.hpp"
#include "who.hpp"
#include "me.hpp"
#include "find.hpp"
#include "warp.hpp"
#include "punch.hpp"
#include "skin.hpp"
#include "news.hpp"
#include "weather.hpp"
#include "ghost.hpp"

#include "__command.hpp"

/* if you plan to use this outside of this file, please include in __command.hpp (^-^) - and just make it a void. */
void help_return(ENetEvent& event, const std::string_view text)
{
    send_gameMessage(event.peer, {.message = "action|log\nmsg|>> Commands: /msg /sb /time /who /me /find /warp /punch /skin /news /weather /ghost /ageworld /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy"});
}

const std::unordered_map<std::string_view, void(*)(ENetEvent&, std::string_view)> cmd_pool
{
    {"/help", help_return},
    {"/?", help_return},
    {"/msg", msg},
    {"/sb", sb},
    {"/time", command::time}, // @note namespace is to prevent mismatching C time
    {"/who", who},
    {"/me", me},
    {"/find", find},
    {"/warp", warp},
    {"/punch", punch},
    {"/skin", skin},
    {"/news", news},
    {"/weather", weather},
    {"/ghost", ghost},
    {"/wave", on::Action}, {"/dance", on::Action}, {"/love", on::Action}, {"/sleep", on::Action}, {"/facepalm", on::Action}, {"/fp", on::Action}, 
    {"/smh", on::Action}, {"/yes", on::Action}, {"/no", on::Action}, {"/omg", on::Action}, {"/idk", on::Action}, {"/shrug", on::Action}, 
    {"/furious", on::Action}, {"/rolleyes", on::Action}, {"/foldarms", on::Action}, {"/fa", on::Action}, {"/stubborn", on::Action}, {"/fold", on::Action}, 
    {"/dab", on::Action}, {"/sassy", on::Action}, {"/dance2", on::Action}, {"/march", on::Action}, {"/grumpy", on::Action}, {"/shy", on::Action}
};
