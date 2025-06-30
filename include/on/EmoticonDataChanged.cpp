#include "pch.hpp"
#include "EmoticonDataChanged.hpp"

std::unordered_map<std::string_view, std::string_view> emoticon = 
{
    {"wl", "\u0101"}, {"yes", "\u0102"}, {"no", "\u0103"}, {"love", "\u0104"}, {"oops", "\u0105"}, {"shy", "\u0106"},
    {"wink", "\u0107"}, {"tongue", "\u0108"}, {"agree", "\u0109"}, {"sleep", "\u010A"}, {"punch", "\u010B"}, {"music", "\u010C"},
    {"build", "\u010D"}, {"megaphone", "\u010E"}, {"sigh", "\u010F"}, {"mad", "\u0110"}, {"wow", "\u0111"}, {"dance", "\u0112"},
    {"see-no-evil", "\u0113"}, {"bheart", "\u0114"}, {"heart", "\u0115"}, {"grow", "\u0116"}, {"gems", "\u0117"}, {"kiss", "\u0118"},
    {"gtoken", "\u0119"}, {"lol", "\u011A"}, {"smile", "\u011B"}, {"cool", "\u011C"}, {"cry", "\u011D"}, {"vend", "\u011E"},
    {"bunny", "\u011F"}, {"cactus", "\u0120"}, {"pine", "\u0121"}, {"peace", "\u0122"}, {"terror", "\u0123"}, {"troll", "\u0124"},
    {"evil", "\u0125"}, {"fireworks", "\u0126"}, {"football", "\u0127"}, {"alien", "\u0128"}, {"party", "\u0129"}, {"pizza", "\u012A"},
    {"clap", "\u012B"}, {"song", "\u012C"}, {"ghost", "\u012D"}, {"nuke", "\u012E"}, {"halo", "\u012F"}, {"turkey", "\u0130"},
    {"gift", "\u0131"}, {"cake", "\u0132"}, {"heartarrow", "\u0133"}, {"lucky", "\u0134"}, {"shamrock", "\u0135"}, {"grin", "\u0136"},
    {"ill", "\u0137"}, {"eyes", "\u0138"}, {"weary", "\u0139"}, {"moyai", "\u013A"}, {"plead", "\u013B"}
};

void on::EmoticonDataChanged(ENetEvent& event)
{
    std::string EmoticonData;
    EmoticonData.reserve(emoticon.size() * 23/* emoticon data + ()||1/0& */);
    for (const auto &[key, value] : emoticon) 
        EmoticonData.append(std::format("({})|{}|1&", key, value)); // @todo add requirements for unlocking emoticons
    packet::create(*event.peer, false, 0, {
        "OnEmoticonDataChanged", 
        201560520, 
        EmoticonData.c_str()
    });
}
