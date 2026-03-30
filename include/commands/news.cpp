#include "pch.hpp"
#include "on/RequestGazette.hpp"

#include "news.hpp"

void news(ENetEvent& event, const std::string_view text)
{
    on::RequestGazette(event);
}
