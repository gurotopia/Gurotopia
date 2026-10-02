#pragma once

namespace action
{ 
    extern void quit_to_exit(ENetEvent& event, const std::string& header, bool skip_selection);
}

/* leaving players stay visible for a moment (their "left" bubble); these finish or send the removal */
extern void leave_flush(const std::string &world, int user_id);
extern void leave_pump();

/* true while a player is quietly re-entered (title change): no "left"/"entered" messages */
extern bool g_silent_move;


