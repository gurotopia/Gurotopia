#include "pch.hpp"
#include "connect.hpp"
#include "disconnect.hpp"
#include "receive.hpp"
#include "disconnect_timeout.hpp"
#include "__event_type.hpp"

std::unordered_map<ENetEventType, std::function<void(ENetEvent)>> event_pool
{
    {::ENET_EVENT_TYPE_CONNECT, std::bind(&_connect, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_DISCONNECT, std::bind(&disconnect, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_RECEIVE, std::bind(&receive, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT, std::bind(&disconnect_timeout, std::placeholders::_1)},
};
