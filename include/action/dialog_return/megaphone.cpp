#include "pch.hpp"

#include "commands/sb.hpp"

#include "megaphone.hpp"

void megaphone(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    if (pipes.size() < 7zu) return;

    sb(event, pipes[5zu]); // @todo handle this when /sb requires gems @todo handle trim
}