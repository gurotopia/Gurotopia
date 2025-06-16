#pragma once
#ifndef EMOTICONDATACHANGED_HPP
#define EMOTICONDATACHANGED_HPP

    extern std::unordered_map<std::string_view, std::string_view> emoticon;

    extern void EmoticonDataChanged(ENetEvent& event);

#endif