#pragma once

extern std::unordered_map<std::string, std::function<void(ENetEvent, const std::string&)>> action_pool;
