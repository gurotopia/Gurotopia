#include "pch.hpp"
#include "commands/friendsys.hpp"
#include "socialportal.hpp"

void socialportal(ENetEvent& event, const ::hPipe &hPipe)
{
    if (hPipe["buttonClicked"] == "showfriend") friends_show(event, false);
}
