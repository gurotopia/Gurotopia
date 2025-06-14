#include "pch.hpp"
#include "type_connect.hpp"
#include "type_disconnect.hpp"
#include "type_receive.hpp"
#include "type_disconnect_timeout.hpp"
#include "__event_type.hpp"

std::unordered_map<ENetEventType, std::function<void(ENetEvent)>> event_pool
{
    {::ENET_EVENT_TYPE_CONNECT, std::bind(&type_connect, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_DISCONNECT, std::bind(&type_disconnect, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_RECEIVE, std::bind(&type_receive, std::placeholders::_1)},
    {::ENET_EVENT_TYPE_DISCONNECT_TIMEOUT, std::bind(&type_disconnect_timeout, std::placeholders::_1)},
};
