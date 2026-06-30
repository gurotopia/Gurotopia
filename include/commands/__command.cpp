#include "pch.hpp"
#include "on/Action.hpp"
#include "find.hpp"
#include "warp.hpp"
#include "punch.hpp"
#include "skin.hpp"
#include "sb.hpp"
#include "who.hpp"
#include "me.hpp"
#include "news.hpp"
#include "weather.hpp"
#include "ghost.hpp"
#include "ageworld.hpp"
#include "__command.hpp"

/* emote commands all dispatch to on::Action. listed once here so the
 * cmd_pool registration and the /help text stay in sync automatically. */
static constexpr std::array<std::string_view, 24zu> emotes{
    "wave", "dance", "love", "sleep", "facepalm", "fp",
    "smh", "yes", "no", "omg", "idk", "shrug",
    "furious", "rolleyes", "foldarms", "fa", "stubborn", "fold",
    "dab", "sassy", "dance2", "march", "grumpy", "shy"
};

/* named commands with their usage hint, shown in /help */
static constexpr std::string_view named_help =
    "/find /warp {world} /punch {id} /skin {id} /sb {msg} /who /me {msg} "
    "/news /weather {id} /ghost /ageworld";

/* if you plan to use this outside of this file, please include in __command.hpp (^-^) - and just make it a void. */
auto help_return = [](ENetEvent& event, const std::string_view text) 
{
    std::string list{ named_help };
    for (std::string_view emote : emotes)
        list += std::format(" /{}", emote);

    send_action(*event.peer, "log", std::format("msg|>> Commands: {} \0", list));
};

std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool = []
{
    std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> pool
    {
        {"help", help_return },
        {"?", help_return },
        {"find", &find},
        {"warp", &warp},
        {"punch", &punch},
        {"skin", &skin},
        {"sb", &sb},
        {"who", &who},
        {"me", &me},
        {"news", &news},
        {"weather", &weather},
        {"ghost", &ghost},
        {"ageworld", &ageworld},
    };

    for (std::string_view emote : emotes)
        pool.emplace(emote, &on::Action);

    return pool;
}();
