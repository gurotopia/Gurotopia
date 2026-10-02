#pragma once

/* one oven that currently has something in it */
struct cooking_pot
{
    std::string world{};
    int x{}, y{};
    std::vector<short> ingredients{};
    short result{};          // @note 0 while still being filled
    std::time_t ready_at{};  // @note when the dish is done
    int owner{};             // @note user_id who started it
};

extern std::vector<::cooking_pot> cooking_pots;

/* called from tile_activate when a COOKING_OVEN is punched */
extern bool cooking_use(ENetEvent& event, ::world &world, int x, int y);

/* called from dialog_return when the ingredient picker comes back */
extern void cooking_dialog(ENetEvent& event, const ::hPipe &hPipe);
