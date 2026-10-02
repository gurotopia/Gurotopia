#include "pch.hpp"
#include "onVariant/Action.hpp"
#include "time.hpp"
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
#include "reset.hpp"
#include "gems.hpp"
#include "give.hpp"
#include "invis.hpp"
#include "mod.hpp"
#include "removeblock.hpp"
#include "nick.hpp"
#include "flag.hpp"
#include "warpto.hpp"
#include "kick.hpp"
#include "private.hpp"
#include "dev.hpp"
#include "nuke.hpp"
#include "unnuke.hpp"
#include "announce.hpp"
#include "showplayer.hpp"
#include "god.hpp"
#include "firework.hpp"
#include "freeze.hpp"
#include "ducttape.hpp"
#include "fill.hpp"
#include "sphere.hpp"
#include "undo.hpp"
#include "worlds.hpp"
#include "deleteworld.hpp"
#include "saveworld.hpp"
#include "setowner.hpp"
#include "stats.hpp"
#include "summon.hpp"
#include "swap.hpp"
#include "list.hpp"
#include "uptime.hpp"
#include "shutdown.hpp"
#include "pull.hpp"
#include "trade.hpp"
#include "unban.hpp"
#include "legendary.hpp"
#include "breakall.hpp"
#include "access.hpp"
#include "renameworld.hpp"
#include "spawnall.hpp"
#include "wear.hpp"
#include "undress.hpp"
#include "heal.hpp"
#include "msg.hpp"
#include "whois.hpp"
#include "motd.hpp"
#include "maintenance.hpp"
#include "clear.hpp"
#include "xp.hpp"
#include "bot.hpp"
#include "search.hpp"
#include "msg.hpp"
#include "whois.hpp"
#include "motd.hpp"
#include "maintenance.hpp"
#include "clear.hpp"
#include "xp.hpp"
#include "bot.hpp"
#include "search.hpp"
#include "curse.hpp"
#include "events.hpp"
#include "worldtools.hpp"
#include "devmenu.hpp"
#include "accounts.hpp"
#include "__command.hpp"

/* emote commands all dispatch to on::Action. listed once here so the
 * cmd_pool registration and the /help text stay in sync automatically. */
static constexpr std::string_view emotes[24]{
    "wave", "dance", "love", "sleep", "facepalm", "fp",
    "smh", "yes", "no", "omg", "idk", "shrug",
    "furious", "rolleyes", "foldarms", "fa", "stubborn", "fold",
    "dab", "sassy", "dance2", "march", "grumpy", "shy"
};

/* named commands with their usage hint, shown in /help */
static constexpr std::string_view player_help = "/time /find /warp {world} /sb {msg} /who /me {msg} /news /uptime /msg {player} {text} /reply {text} /pull {player} /trade {player} /title [title/on/off] /flag [code]";
static constexpr std::string_view mod_help = "/warpto {player} /kick {player} /private /announce {msg} /freeze {player} /ducttape [player] /worlds /stats /summon {player} /swap {player} /list /spawnall /whois {player} /clear /curse {player} {time} {reason} /uncurse {player} /unban {player} /ghost";
static constexpr std::string_view dev_help = "/skin {id} /reset /gems {amount} /give {id} {amount} /invis /mod {name} {0-2} /removeblock /nick {name} /nuke /unnuke /showplayer /god /firework [count] /fill {id} /sphere {id} {r} /undo [n] /deleteworld {name} /save /setowner [player] /shutdown [seconds] /breakall /access [player] /renameworld {name} /wear {id} /motd [text] /maintenance /xp {amount} /level [n] /bot [add/remove/clear/say] /search /punch {id} /weather {id} /dev /undress /heal /event [number] /tile [x y] /deleteallworlds [keep...] /fillworld /cleardrops /accounts [search] /title [player] [title] [off]";

std::array<std::string_view, 27> cmd_requires_arg{
    "sb", "warp", "punch", "skin", "me", "weather", "gems", "give", "mod", "warpto", "kick", "dev", "announce", "freeze", "fill", "sphere", "deleteworld", "summon", "swap", "renameworld", "wear", "msg", "reply", "whois", "xp", "pull", "unban"
};

/* if you plan to use this outside of this file, please include in __command.hpp (^-^) - and just make it a void. */
auto help_return = [](ENetEvent& event, const std::string_view text) 
{
    if (static_cast<::peer*>(event.peer->data)->role >= DEVELOPER) { devmenu_open(event); return; } // @note popup for devs
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    std::string list{ player_help };
    if (pPeer->role >= MODERATOR && !mod_help.empty()) list += std::format(" {}", mod_help);
    if (pPeer->role >= DEVELOPER && !dev_help.empty()) list += std::format(" {}", dev_help);
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
        {"time", &command::time}, // @note namespace is to prevent mismatching C time
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
        {"reset", &reset},
        {"gems", &gems},
        {"give", &give},
        {"invis", &invis},
        {"mod", &mod},
        {"removeblock", &removeblock},
        {"nick", &nick},
    {"flag", &flag_cmd},
        {"warpto", &warpto},
        {"kick", &kick},
        {"private", &private_cmd},
        {"dev", &dev},
        {"nuke", &nuke},
        {"unnuke", &unnuke},
        {"announce", &announce},
        {"showplayer", &showplayer},
        {"god", &god},
        {"firework", &firework},
        {"freeze", &freeze},
        {"ducttape", &ducttape},
        {"fill", &fill},
        {"sphere", &sphere},
        {"undo", &undo},
        {"worlds", &worlds_cmd},
        {"deleteworld", &deleteworld},
        {"save", &saveworld},
        {"setowner", &setowner},
        {"stats", &stats},
        {"summon", &summon},
        {"swap", &swap_cmd},
        {"list", &list_cmd},
        {"uptime", &uptime},
        {"shutdown", &shutdown_cmd},
        {"pull", &pull_cmd},
        {"trade", &trade_cmd},
        {"unban", &unban_cmd},
        {"title", &title_cmd},
        {"breakall", &breakall},
        {"access", &access_cmd},
        {"renameworld", &renameworld},
        {"spawnall", &spawnall},
        {"wear", &wear},
        {"undress", &undress},
        {"heal", &heal},
        {"msg", &msg},
        {"reply", &reply},
        {"whois", &whois},
        {"motd", &motd},
        {"maintenance", &maintenance},
        {"clear", &clear_cmd},
        {"xp", &xp_cmd},
        {"level", &level_cmd},
        {"bot", &bot_cmd},
        {"search", &search_cmd},
        {"curse", &curse_cmd},
        {"uncurse", &uncurse_cmd},
        {"event", &event_cmd},
        {"tile", &tile_cmd},
        {"dumpitems", &dumpitems_cmd},
        {"deleteallworlds", &deleteallworlds_cmd},
        {"fillworld", &fillworld_cmd},
        {"cleardrops", &cleardrops_cmd},
        {"accounts", &accounts_cmd}
    };

    for (std::string_view emote : emotes)
        pool.emplace(emote, &on::Action);

    return pool;
}();


std::string_view help_for(int level) { return level == 0 ? player_help : level == 1 ? mod_help : dev_help; }
