#pragma once

/* ---- titles: "of Legend" (Quest For Honor) and "Dr." ---- */
extern void legend_load(const ::peer &p);            // @note at login
extern void legend_save(const ::peer &p);            // @note at logout
extern void legend_tick(std::time_t now);            // @note saves quest progress now and then
extern std::string title_name(const ::peer &p, const std::string &base); // @note adds " of Legend" when that title is on
extern std::string country_state_for(const ::peer &p);                   // @note "se|maxLevel|doctor"
extern void titles_open(ENetEvent &event);            // @note wrench yourself > Title
extern void titles_return(ENetEvent &event, const ::hPipe &hPipe);
extern std::string title_quick_button(const ::peer &p); // @note one-tap on/off in your own wrench menu
extern void title_toggle(ENetEvent &event);
extern void title_cmd(ENetEvent &event, const std::string_view text); // @note /title {player} legend|dr

/* ---- Legendary Quests at the Legendary Wizard ---- */
extern bool wizard_wrench(ENetEvent &event, ::world &world, int x, int y); // @return true if that tile is a Legendary Wizard
extern void wizard_return(ENetEvent &event, const ::hPipe &hPipe);
extern void legend_on_break(ENetEvent &event, int rarity);
extern void legend_on_plant(ENetEvent &event, int rarity);
extern void legend_on_harvest(ENetEvent &event, int fruit_rarity);
extern void legend_on_xp(ENetEvent &event, int xp);

/* items normal players can't pick in /find (wands, quest rewards) */
extern bool staff_only_item(int id);
