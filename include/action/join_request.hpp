#pragma once

namespace action
{ 
    extern void join_request(ENetEvent& event, const std::string& header, const std::string_view world_name);
}

/* the next time this account enters a world it appears here instead of at the door (summon, pull) */
extern void arrive_at(int user_id, ::pos at);

