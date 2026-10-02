#pragma once

extern void accounts_cmd(ENetEvent &event, const std::string_view text);
extern void accounts_return(ENetEvent &event, const ::hPipe &hPipe);
