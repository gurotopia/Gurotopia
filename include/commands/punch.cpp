#include "pch.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "database/items.hpp"
#include "punch.hpp"

/* punch effects by item name, for items not in get_punch_id's switch.
 * names are compared lowercase, letters and digits only. */
static std::string punch_norm(std::string_view s)
{
    std::string out{};
    for (char c : s)
        if (std::isalnum(static_cast<unsigned char>(c))) out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

static u_char punch_by_name(u_int item_id)
{
    static std::unordered_map<u_int, u_char> by_id{};
    static bool built = false;
    if (!built)
    {
        built = true;
        static const std::pair<u_char, const char*> table[]{
            {1, "Cyclopean Visor"}, {2, "Heart Bow"}, {3, "Tommy Gun"}, {4, "Elvish Longbow"},
            {5, "Sawed-Off Shotgun"}, {6, "Dragon Hand"}, {7, "Reanimator Remote"},
            {8, "Death Ray"}, {9, "Six Shooter"}, {10, "Focused Eyes"}, {11, "Ice Dragon Hand"},
            {13, "Atomic Shadow Scythe"}, {14, "Pet Leprechaun"}, {15, "Battle Trout"},
            {16, "Fiesta Dragon"}, {17, "Squirt Gun"}, {18, "Keytar"}, {19, "Flamethrower"},
            {20, "Legendbot-009"}, {21, "Dragon of Legend"}, {22, "Zeus' Lightning Bolt"}, {23, "Violet Protodrake Leash"}, {24, "Ring of Force"}, {25, "Ice Calf Leash"}, {28, "Ecto Pack"}, {29, "Carrot Sword"}, {30, "Claw Glove"},
            {31, "Cosmic Unicorn Bracelet"}, {32, "Black Crystal Dragon"}, {33, "Mighty Snow Rod"}, {34, "Tiny Tank"},
            {35, "Crystal Glaive"}, {36, "Heavenly Scythe"}, {37, "Heartbreaker Hammer"}, {38, "Diamond Dragon"},
            {39, "Burning Eyes"}, {40, "Diamond Horns"}, {41, "Marshmallow Basket"}, {42, "Flame Scythe"},
            {43, "Legendary Katana"}, {44, "Electric Bow"}, {45, "Pineapple Launcher"}, {46, "Demonic Arm"},
            {47, "The Gungnir"}, {49, "Poseidon's Trident"},
            {50, "Wizard's Staff"}, {53, "Tennis Racquet"},
            {54, "Baseball Glove"}, {55, "Basketball"}, {57, "Fire Hose"},
            {58, "Soul Orb"}, {59, "Strawberry Slime"}, {60, "Axe of Winter"}, {62, "T-Shirt Cannon"},
            {64, "Party Blaster"}, {65, "Serpent Staff"}, {66, "Spring Bouquet"},
            {68, "Toy Lock-Bot"}, {69, "Neutron Gun"}, {71, "Solsascarf"}, {72, "Skull Launcher"},
            {73, "AK-8087"}, {76, "Adventurer's Whip"}, {77, "Burning Hands"},
            {78, "Balloon Launcher"}, {80, "Rayman's Fist"}
        };

        std::unordered_map<std::string, u_char> by_name{};
        for (const auto &[fx, name] : table) by_name.emplace(punch_norm(name), fx);

        std::vector<std::string> matched{};
        for (const ::item &it : items)
            if (auto f = by_name.find(punch_norm(it.raw_name)); f != by_name.end())
            {
                by_id.emplace(static_cast<u_int>(it.id), f->second);
                matched.emplace_back(f->first);
            }

        for (const auto &[fx, name] : table)
            if (std::ranges::find(matched, punch_norm(name)) == matched.end())
                (void)0; // @note debug line removed
        (void)0; // @note debug line removed
    }
    const auto f = by_id.find(item_id);
    return (f != by_id.end()) ? f->second : 0;
}
/* item -> punch effect, from GrowServer (github.com/StileDevs/GrowServer, MIT License). checked first. */
static const std::unordered_map<u_int, u_char> punch_table{
    {8006,19}, {8008,19}, {8010,19}, {8012,19}, {4748,40}, {2596,43}, {4746,75}, {7216,94}, {9348,119}, {8372,121}, 
    {10330,178}, {11116,220}, {11250,228}, {11814,241}, {1780,20}, {7408,61}, {6892,87}, {11760,237}, {11548,242}, {11552,242}, 
    {11704,245}, {11706,245}, {2220,34}, {2878,52}, {2880,52}, {6298,84}, {6966,88}, {7384,98}, {7950,113}, {9136,139}, 
    {9138,140}, {11308,200}, {10890,202}, {10990,205}, {11140,225}, {10128,171}, {10336,39}, {11142,223}, {11506,245}, {11508,245}, 
    {11562,245}, {11768,245}, {11882,245}, {1204,10}, {138,1}, {1952,26}, {2476,39}, {5002,19}, {5006,56}, {7402,100}, 
    {7836,112}, {7838,112}, {7840,112}, {7842,112}, {8816,128}, {8818,128}, {8820,128}, {8822,128}, {366,2}, {1464,2}, 
    {472,3}, {594,4}, {10130,4}, {5424,4}, {5456,4}, {4136,4}, {768,5}, {900,6}, {7760,6}, {9272,6}, 
    {7758,6}, {910,7}, {930,8}, {1010,8}, {6382,8}, {1016,9}, {1378,11}, {1484,13}, {1512,14}, {1648,14}, 
    {1542,15}, {1576,16}, {1676,17}, {7504,17}, {1710,18}, {4644,18}, {1714,18}, {1712,18}, {6044,18}, {1570,18}, 
    {1748,19}, {1782,21}, {1804,22}, {1868,23}, {1998,23}, {1874,24}, {1946,25}, {2800,25}, {2854,26}, {1956,27}, 
    {2908,29}, {6312,29}, {8554,29}, {3162,29}, {4956,29}, {3466,29}, {4166,29}, {4506,29}, {2952,29}, {3932,29}, 
    {3934,29}, {8732,29}, {3108,29}, {1980,30}, {2066,31}, {11082,31}, {11080,31}, {11078,31}, {2212,32}, {2218,33}, 
    {2266,35}, {2386,36}, {2388,37}, {2450,38}, {2512,41}, {2572,42}, {2592,43}, {9396,43}, {2720,44}, {2752,45}, 
    {2754,46}, {2756,47}, {2802,49}, {2866,50}, {2876,51}, {2906,53}, {4170,53}, {2886,54}, {2890,55}, {2910,56}, 
    {3066,57}, {3124,58}, {3168,59}, {3214,60}, {9194,60}, {3238,61}, {3274,62}, {3300,64}, {3418,65}, {3476,66}, 
    {3596,67}, {3686,68}, {3716,69}, {4474,72}, {4464,73}, {4778,76}, {6026,76}, {4996,77}, {3680,77}, {4840,78}, 
    {5480,80}, {6110,81}, {6308,82}, {6310,83}, {6756,85}, {7044,86}, {7088,89}, {11020,89}, {7098,90}, {9032,90}, 
    {9738,92}, {3166,93}, {9340,95}, {7392,96}, {7414,99}, {7424,101}, {7470,102}, {7488,103}, {7586,104}, {7646,104}, 
    {7650,105}, {6804,106}, {7568,107}, {7570,107}, {7572,107}, {7574,107}, {7668,108}, {7660,109}, {9060,109}, {7736,111}, 
    {9116,111}, {9118,111}, {7826,111}, {7828,111}, {11440,111}, {11442,111}, {11312,111}, {7830,111}, {7832,111}, {10670,111}, 
    {9120,111}, {9122,111}, {10680,111}, {10626,111}, {10578,111}, {10334,111}, {11380,111}, {11326,111}, {7912,111}, {11298,111}, 
    {10498,111}, {8002,114}, {8022,116}, {8036,118}, {8038,120}, {8910,129}, {8942,130}, {8944,131}, {5276,131}, {8432,132}, 
    {8434,132}, {8436,132}, {8950,132}, {8946,133}, {8960,134}, {9058,136}, {9082,137}, {9304,137}, {9066,138}, {9256,144}, 
    {9236,145}, {9342,146}, {9378,148}, {9410,150}, {9606,152}, {9716,153}, {10064,168}, {10046,169}, {10050,170}, {10388,180}, 
    {10442,184}, {10506,185}, {10652,188}, {10676,191}, {10694,193}, {10714,194}, {10724,195}, {10722,196}, {10888,199}, {10886,200}, 
    {10922,203}, {10998,206}, {10952,207}, {11000,208}, {11006,209}, {11052,211}, {10960,212}, {10956,213}, {10958,214}, {10954,215}, 
    {11076,216}, {11084,217}, {11118,218}, {11120,219}, {11158,221}, {11162,222}, {11248,226}, {11240,227}, {11284,229}, {11292,231}, 
    {11316,234}, {11354,236}, {11464,237}, {11438,237}, {11716,237}, {11718,237}, {11674,237}, {11630,237}, {11786,237}, {11872,237}, 
    {1960,28}, {5206,79}, {7196,95}, {9006,135}, {9172,141}, {10210,172}, {10754,197}, {11046,210}, {11314,233}, {1440,12}, 
    {4208,39}, {9462,151}, {4150,31}, {4290,71}, {7192,91}, {7136,92}, {9254,143}, {9376,149}, {11232,224}, {11324,235}, 
    {11818,248}, {11876,248}
};
u_char get_punch_id(u_int item_id)
{
    if (const auto t = punch_table.find(item_id); t != punch_table.end()) return t->second;
    switch (item_id)
    {
        case 138: case 2976: return 1; // @note https://growtopia.fandom.com/wiki/Mods/Eye_Beam
        case 366: return 2;
        case 472: return 3;
        case 594: return 4;
        case 768: return 5;
        case 900: return 6;

        case 930: return 8; // @todo or 12

        case 1204: return 10;
        case 1738: return 11;
        case 1484: return 12;

        case 3066: case 5206: case 7504: case 10288: return 17; // @note https://growtopia.fandom.com/wiki/Mods/Fire_Hose
        case 2636: case 2908: case 3070: case 3108: case 3466: case 2952: return 29; // @note https://growtopia.fandom.com/wiki/Mods/Slasher

        default: return punch_by_name(item_id);
    }
}

void punch(ENetEvent& event, const std::string_view text) 
{
    const std::string id{ text.substr(sizeof("punch ")-1) };
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->punch_effect = (u_char)atoi(id.c_str());
    on::SetClothing(*event.peer);
}