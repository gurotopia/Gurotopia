#pragma once

namespace buff
{
    enum id : u_char { XP_DOUBLE = 1, XP_TRIPLE, XP_BOOST, CONSUME_XP, GEM_CHANCE, GEM_DOUBLE, TREE_GROW, LUCKY, SPEEDY, HIGH_JUMP, PUNCH, SPIKE, GAZPACHO, PURE_LOVE };
}

extern bool        buff_active(const ::peer &p, u_char id);
extern u_short     buff_xp(const ::peer &p, u_short value);
extern u_short     consumable_xp(const ::peer &p);
extern void        buffs_tick(std::time_t now);
extern std::string buffs_wrench(const ::peer &p);
