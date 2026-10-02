#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/bubble.hpp"
#include "tools/create_dialog.hpp"
#include "database/items.hpp"
#include "__command.hpp"
#include "legendary.hpp"
#include "devmenu.hpp"

/* ------------------------------------------------------------------ descriptions (edit freely) */
static const std::unordered_map<std::string_view, const char*> descriptions{
    { "time",            "Shows the server time." },
    { "find",            "Search any item and add it to your backpack." },
    { "warp",            "Travel to a world." },
    { "punch",           "Change your punch effect by number." },
    { "skin",            "Change your skin color." },
    { "sb",              "Super-broadcast a message to everyone online." },
    { "who",             "Lists the players in your world." },
    { "me",              "Say an action in the third person." },
    { "news",            "Shows the news." },
    { "weather",         "Changes the weather you see." },
    { "ghost",           "Walk through blocks." },
    { "dev",             "Become a developer with the password." },
    { "uptime",          "How long the server has been running." },
    { "undress",         "Takes off all your clothes." },
    { "heal",            "Restores your health." },
    { "msg",             "Send a private message to a player." },
    { "reply",           "Reply to your last private message." },
    { "god",             "Nothing can hurt you; walk on spikes and lava." },
    { "curse",           "Sends a player to HELL for a while." },
    { "uncurse",         "Lifts a player's curse early." },
    { "freeze",          "Freezes or unfreezes a player." },
    { "summon",          "Pulls a player to you." },
    { "kick",            "Kicks a player." },
    { "nick",            "Changes your displayed name." },
    { "flag",            "Changes the flag beside your name." },
    { "event",           "Starts a special event in your world." },
    { "tile",            "Shows what the server stores on a tile." },
    { "dumpitems",       "Writes every item name to items_list.txt." },
    { "deleteallworlds", "Deletes every world except the ones kept." },
    { "fillworld",       "Fills the world with dropped items." },
    { "cleardrops",      "Removes every dropped item in the world." },
    { "bot",             "Adds, removes or controls bots." },
    { "nuke",            "Nukes a world." },
    { "xp",              "Gives you XP." },
    { "level",           "Sets your level." },
    { "save",            "Saves all worlds." },
    { "accounts",        "Lists every account, with details and delete." },
    { "shutdown",        "Countdown in chat, then saves and stops the server." },
    { "pull",            "Pulls a player in your world to where you stand." },
    { "trade",           "Starts a trade with a player in your world." },
    { "unban",           "Lifts a ban so the player can log in again." },
    { "title",           "Gives or takes the of Legend / Dr. title." },
};

/* ------------------------------------------------------------------ help text parsing */
struct cmd_info
{
    std::string name;
    std::vector<std::string> args;      // @note labels without brackets
    std::vector<bool> required;
    std::string usage;                  // @note "/name {a} [b]"
    int level;
};

static std::vector<cmd_info> all_commands()
{
    std::vector<cmd_info> out{};
    for (int level : { 2, 1, 0 })
    {
        std::string help{ help_for(level) };
        std::string word{};
        cmd_info *cur = nullptr;
        for (char ch : help + ' ')
        {
            if (ch != ' ') { word += ch; continue; }
            if (word.empty()) continue;
            if (word.front() == '/')
            {
                out.push_back({ word.substr(1), {}, {}, word, level });
                cur = &out.back();
            }
            else if (cur)
            {
                const bool req = word.front() == '{';
                std::string label = word;
                std::erase_if(label, [](char x) { return x == '{' || x == '}' || x == '[' || x == ']'; });
                cur->args.push_back(label);
                cur->required.push_back(req);
                cur->usage += " " + word;
            }
            word.clear();
        }
    }
    return out;
}

static const char *describe(const std::string &name)
{
    const auto d = descriptions.find(name);
    return d == descriptions.end() ? "" : d->second;
}

/* ------------------------------------------------------------------ /? menu */
void devmenu_open(ENetEvent &event)
{
    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", "`wCommands``", 32)
     .add_smalltext("Tap a command to use it. Commands that need more info open a form first.");

    const std::vector<cmd_info> cmds = all_commands();
    for (int level : { 2, 1, 0 })
    {
        bool header = false;
        for (const cmd_info &c : cmds)
        {
            if (c.level != level) continue;
            if (!header)
            {
                d.add_spacer("small").add_label("big", level == 2 ? "`4Developer``" : level == 1 ? "`5Moderator``" : "`2Everyone``");
                header = true;
            }
            const char *desc = describe(c.name);
            d.add_button("run_" + c.name, "/" + c.name);
            d.add_smalltext(*desc ? std::format("`w{}`` - {}", c.usage, desc) : std::format("`w{}``", c.usage));
        }
    }
    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("dev_menu", "Close", "") });
}

/* form with one box per argument. @return true if a form was opened */
bool devmenu_form(ENetEvent &event, const std::string &name, bool only_if_required)
{
    const std::vector<cmd_info> cmds = all_commands();
    const auto c = std::ranges::find(cmds, name, &cmd_info::name);
    if (c == cmds.end() || c->args.empty()) return false;
    if (only_if_required && std::ranges::none_of(c->required, [](bool r) { return r; })) return false;

    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", std::format("`w/{}``", c->name), 32);
    if (const char *desc = describe(c->name); *desc) d.add_smalltext(desc);
    d.add_smalltext(std::format("Usage: `w{}``", c->usage));
    for (std::size_t i = 0; i < c->args.size(); ++i)
        d.add_text_input(std::format("a{}", i), std::format("{}{}:", c->args[i], c->required[i] ? "" : " (optional)"), std::string{}, 60);
    d.embed_data("cmd", c->name);
    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("cmd_form", "Cancel", "Run") });
    return true;
}

static void run_command(ENetEvent &event, const std::string &text)
{
    const std::string name = text.substr(0, text.find(' '));
    const auto it = cmd_pool.find(name);
    if (it == cmd_pool.end()) { tell(event.peer, "`4That command doesn't exist.``"); return; }
    send_action(*event.peer, "log", std::format("msg| `6/{}``", text).c_str());
    it->second(event, text);
}

void dev_menu_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;
    const std::string btn = hPipe["buttonClicked"];
    if (!btn.starts_with("run_")) return;
    const std::string name = btn.substr(4);
    if (devmenu_form(event, name, false)) return;
    run_command(event, name);
}

void cmd_form_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) return;
    std::string text = hPipe["cmd"];
    if (text.empty()) return;
    for (int i = 0; i < 10; ++i)
    {
        std::string v = hPipe[std::format("a{}", i)];
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) v.erase(0, 1);
        if (!v.empty()) text += " " + v;
    }
    run_command(event, text);
}

/* ------------------------------------------------------------------ /find item card */
void find_item_popup(ENetEvent &event, int id)
{
    if (id <= 0 || id >= static_cast<int>(items.size())) return;
    const ::item &it = id_to_item(static_cast<u_short>(id));
    std::string info = it.info.empty() ? std::string{ "No description." } : it.info;
    for (char &ch : info) if (ch == '|' || ch == '\n' || ch == '\r') ch = ' ';

    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", std::format("`w{}``", it.raw_name), id)
     .add_smalltext(std::format("ID `w{}``   Rarity `w{}``", id, it.rarity))
     .add_textbox(info)
     .embed_data("item", id)
     .add_button("get_1", "Get 1")
     .add_button("get_10", "Get 10")
     .add_button("get_100", "Get 100")
     .add_button("get_200", "Get 200");
    if (id + 1 < static_cast<int>(items.size()) && id_to_item(static_cast<u_short>(id + 1)).type == type::SEED)
        d.add_button("seed_200", "Get 200 Seeds");
    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("find_give", "Close", "") });
}

void find_give_return(ENetEvent &event, const ::hPipe &hPipe)
{
    const int id = std::atoi(hPipe["item"].c_str());
    const std::string btn = hPipe["buttonClicked"];
    if (id <= 0 || id >= static_cast<int>(items.size())) return;

    int give = id, amount = 0;
    if (btn == "get_1") amount = 1;
    else if (btn == "get_10") amount = 10;
    else if (btn == "get_100") amount = 100;
    else if (btn == "get_200") amount = 200;
    else if (btn == "seed_200") { give = id + 1; amount = 200; }
    if (amount == 0) return;
    if (static_cast<::peer*>(event.peer->data)->role < MODERATOR && staff_only_item(give))
    {
        tell(event.peer, "`4That item is for staff only.``");
        return;
    }

    modify_item_inventory(event, ::slot(static_cast<short>(give), static_cast<short>(amount)));
    tell(event.peer, std::format("`2Added `w{}x {}``!``", amount, id_to_item(static_cast<u_short>(give)).raw_name));
}
