#include "database/peer.hpp"
#include "action/quit.hpp"
#include "type_disconnect.hpp"

void type_disconnect(ENetEvent event)
{
    quit(event, "");
}
