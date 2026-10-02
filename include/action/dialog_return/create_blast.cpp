#include "pch.hpp"
#include "commands/blasts.hpp"

#include "create_blast.hpp"

void create_blast(ENetEvent& event, const ::hPipe &hPipe)
{
    blast_create(event, atoi(hPipe["id"].c_str()), hPipe["name"]);
}
