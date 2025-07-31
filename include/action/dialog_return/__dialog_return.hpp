#pragma once
#ifndef __DIALOG_RETURN_HPP
#define __DIALOG_RETURN_HPP

    extern std::unordered_map<std::string, std::function<void(ENetEvent&, const std::vector<std::string> &&)>> dialog_return_pool;

#endif