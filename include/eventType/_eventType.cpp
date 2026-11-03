#include "pch.hpp"

#include "connect.hpp"
#include "disconnect.hpp"
#include "receive.hpp"

#include "_eventType.hpp"

void event_type_none(ENetEvent& event) {}

const std::array<void(*)(ENetEvent&), 4> eventType_pool = {
    &event_type_none,
    &_connect,
    &disconnect,
    &receive
};
