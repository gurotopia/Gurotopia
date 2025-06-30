#pragma once
#ifndef PACKET_HPP
#define PACKET_HPP

    #include <any>

    /*
    @param p short for peer, the peer who will receive this packet, this can also be used with peers() to send to multiple peers.
    @param netid to my knowledge this value should be true if it relates to a peer's state in a world (OnRemove, OnSpawn OnChangeSkin, ect), else false (OnConsoleMessage, OnTalkBubble, ect.). 
    @param params list of param that structures your packet. each entry will be than identified as a const char* or signed/unsigned or float/double, 
                    respectfully void* entires will not be accepted. e.g. classes, ptr, void
    */
    extern void gt_packet(ENetPeer& p, bool netid, signed delay, const std::vector<std::any>& params);

    extern void _action(ENetPeer& p, const std::string& action, const std::string& str);

#endif