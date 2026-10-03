#include "pch.hpp"

#include "commands/sb.hpp"

#include "megaphone.hpp"

void megaphone(ENetEvent& event, const ::hPipe &hPipe)
{
    const std::string message = hPipe["message"];
    if (message.empty()) return;
    sb(event, "sb " + message); // @note sb() skips the first 3 chars ("sb ") @todo handle this when /sb requires gems @todo handle trim
}