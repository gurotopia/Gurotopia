#include "pch.hpp"

#include "EventButtonDataSet.hpp"

void on::EventButtonDataSet(ENetEvent& event, bool active)
{
    send_varlist(event.peer, { 
        (active) ? "AuctionButton" : "ActiveAuctionButton",
        (active) ? 1 : 0,
        "{" +
            std::format(
                "\"active\":{},\"buttonAction\":\"{}\",\"buttonState\":0,\"buttonTemplate\":\"{}\",\"counter\":0,\"counterMax\":{},\"itemIdIcon\":0,\"name\":\"{}\",\"notification\":0,\"order\":30,\"rcssClass\":\"{}\",\"text\":\"{}\",\"visibilityFlag\":{}",
                (active) ? "true" : "false",
                (active) ? "show_auction_ui" : "", 
                (active) ? "BaseEventButton" : "ActiveAuctionEventButton",
                (active) ? "0" : "20",
                (active) ? "AuctionButton" : "ActiveAuctionButton",
                (active) ? "auction" : "activeauction", 
                (active) ? "Find Auction" : "", 
                (active) ? "10" : "0") 
        + "}"
    });
}
