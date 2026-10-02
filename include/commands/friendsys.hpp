#pragma once

/* friends: wrench > Add as friend, Social Portal > Show Friends */
extern void friend_request(ENetEvent &event, ENetPeer &target);
extern void friends_show(ENetEvent &event, bool edit);
extern void friends_return(ENetEvent &event, const ::hPipe &hPipe); // @note dialogs "friend_request" and "friends_list"
extern std::string friends_welcome(const ::peer &p);                // @note "2 friends are online."
extern void friends_alert(const ::peer &p, bool logged_on);         // @note tells online friends
