#pragma once
#ifndef ITEMS_HPP
#define ITEMS_HPP

    enum clothing : u_char {
        hair, shirt, legs, 
        feet, face, hand, 
        back, head, charm, 
        ances, none
    };

    enum type : u_char {
        FIST = 00,
        WRENCH = 01,
        DOOR = 02,
        LOCK = 03,
        GEM = 04,
        TREASURE = 05,
        DEADLY = 06,
        TRAMPOLINE = 07,
        CONSUMEABLE = 0x08,
        ENTRANCE = 0x09,
        SIGN = 0x0a,
        SFX_BLOCK = 0x0b,
        TOGGLEABLE_ANIMATED_BLOCK = 0x0c,
        MAIN_DOOR = 0x0d,
        PLATFORM = 0x0e,
        STRONG = 0x0f, // @note too strong to break
        FIRE_PAIN = 0x10,
        FOREGROUND = 0x11,
        BACKGROUND = 0x12,
        SEED = 0x13,
        CLOTHING = 0x14,
        ANIMATED = 0x15,
        SFX_BACKGROUND = 0x16,
        ART_WALL = 0x17,
        BOUNCY = 0x18,
        STING_PAIN = 0x19,
        PORTAL = 0x1a,
        CHECKPOINT = 0x1b,
        MUSIC_SHEET = 0x1c,
        SLIPPERY = 0x1d,
        TOGGLEABLE_BLOCK = 0x1f,
        CHEST = 0x20,
        MAILBOX = 0x21,
        BULLETIN = 0x22,
        PINATA = 0x23,
        RANDOM = 0x24,
        COMPONET = 0x25,
        PROVIDER = 0x26,
        CHEMICAL_COMBINER = 0x27,
        ACHIEVEMENT = 0x28,
        WEATHER_MACHINE = 0x29,
        SCOREBOARD = 0x2a,
        SUNGATE = 0x2b,
        TOGGLEABLE_DEADLY = 0x2d,
        HEART_MONITOR = 0x2e,
        DONATION_BOX = 0x2f,
        MANNEQUIN = 0x31,
        CCTV = 0x32,
        MAGIC_EGG = 0x33,
        GAME_BLOCK = 0x34,
        GAME_GENERATOR = 0x35,
        XENONITE = 0x36,
        BOOTH = 0x37,
        CRYSTAL = 0x38,
        CRIME_IN_PROGRESS = 0x39,
        GRINDER = 0x3a,
        SPOTLIGHT = 0x3b,
        PUSHING_BLOCK = 0x3c,
        DISPLAY_BLOCK = 0x3d,
        VENDING_MACHINE = 0x3e,
        FISH_TANK_PORT = 0x3f,
        FISH = 0x40,
        SOLAR_COLLECTOR = 0x41,
        FORGE = 0x42,
        GIVING_TREE = 0x43,
        GIVING_TREE_STUMP = 0x44,
        STEAM_BLOCK = 0x45,
        STEAM_VENT = 0x46,
        STEAM_ORGAN = 0x47,
        SILKWORM = 0x48,
        SEWING_MACHINE = 0x49,
        COUNTRY_FLAG = 0x4a,
        LOBSTER_TRAP = 0x4b,
        PAINTING_EASEL = 0x4c,
        BATTLE_PET_CAGE = 0x4d,
        PET_TRAINER = 0x4e,
        STEAM_ENGINE = 0x4f,
        LOCK_BOT = 0x50,
        SFX_WEATHER_MACHINE = 0x51, // @note Heatwave, Balloon Warz, Background, Valetine's, St.Paddy, Digital Rain Weather Machines
        SPIRIT_STORAGE = 0x52,
        DISPLAY_SHELF = 0x53,
        VIP_ENTRANCE = 0x54,
        CHALLENGE_TIMER = 0x55,
        CHALLENGE_FLAG = 0x56,
        FISH_MOUNT = 0x57,
        PORTRAIT = 0x58,
        SPRITE_WEATHER_MACHINE = 0x59, // @note Stuff, Guild, Epoch Weather Machines,
        FOSSIL = 0x5a,
        FOSSIL_PREP_STATION = 0x5b,
        DNA_PROCESSOR = 0x5c,
        TRICKSTER = 0x5d,
        VALHOWLA_TREASURE = 0x5e,
        CHEMSYNTH_PROCESSOR = 0x5f,
        CHEMSYNTH_TANK = 0x60,
        STORAGE_BOX = 0x61,
        COOKING_OVEN = 0x62,
        AUDIO_BLOCK = 0x63,
        GEIGER_CHARGER = 0x64,
        THE_ADVENTURE_BEGINS = 0x65,
        TOMB_ROBBER = 0x66,
        BALLOON_O_MATIC = 0x67,
        TEAM_ENTRANCE_PUNCH = 0x68,
        TEAM_ENTRANCE_GROW = 0x69,
        TEAM_ENTRANCE_BUILD = 0x6a,
        AURA = 0x6b, // @note including ances
        LEMON_JELLY_BLOCK = 0x6c,
        TRAINING_PORT = 0x6d,
        FISHING_BLOCK = 0x6e,
        MAGPLANT = 0x6f,
        MAGPLANT_REMOTE = 0x70,
        CYBLOCK_BOT = 0x71,
        CYBLOCK_COMMAND = 0x72,
        LUCKY_TOKEN = 0x73,
        GROWSCAN = 0x74,
        CONTAINMENT_FIELD_POWER_NODE = 0x75,
        SPIRIT_BOARD = 0x76,
        WORLD_ARCHITECT = 0x77,
        STARTOPIA_BLOCK = 0x78,
        TOGGLEABLE_MULTI_FRAME_BLOCK = 0x7a,
        AUTO_BREAK_BLOCK = 0x7b,
        AUTO_BREAK_TREE = 0x7c,
        AUTO_BREAK = 0x7d,
        STORM_CLOUD = 0x7e,
        DISAPPEAR_WHEN_STEPPED_ON = 0x7f,
        PUDDLE_BLOCK = 0x80,
        ROOT_CUTTING = 0x81,
        SAFE_VAULT = 0x82,
        ANGELIC_COUNTING_CLOUD = 0x83,
        MINING_EXPLOSIVE = 0x84,
        INFINITY_WEATHER_MACHINE = 0x86,
        GHOST_BLOCK = 0x87,
        ACID = 0x88,
        WAVING_INFLATABLE_ARM_GUY = 0x8a,
        PINEAPPLE_GUZZLER = 0x8c,
        KRANKEN_GALACTIC = 0x8d,
        FRIEND_ENTRANCE = 0x8e,
    };

    enum cat : u_char {
        CAT_RETURN =          02, // @note This item can't be destroyed - smashing it will return it to your backpack if you have room!
        CAT_SUPRISING_FRUIT = 0x08, // @note A tree of this type can bear surprising fruit!
        CAT_PUBLIC =          0x10, // @note This item is PUBLIC: Even if it's locked, anyone can smash it.
        CAT_HOLIDAY =         0x40, // @note This item can only be created during WinterFest/Halloween.
        CAT_UNTRADEABLE =     0x80  // @note This item cannot be dropped or traded.
    };

    #include <string>

    enum collision : u_char {
        NO_COLLISION,
        FULL,
        ON_TOP, // @note platforms
        IF_ACCESS, // @note entrance
        TOGGLE,
        HORIZONTAL,
        IF_VIP, // @note VIP entrance
        VERTICAL,
        ADVENTURE_ITEM,
        ACTIVATE,
        BALLOON_WARZ_TEAM, // @note team entrance
        IF_GUILD, // @note guild entrance
        STEP_ON
    };

    class item 
    {
    public:
        u_short id{}; /* item identity */
        u_char property{};
        u_char cat{};
        u_char type{};
        std::string raw_name{}; /* the exact name of the item including uppercases */
        int ingredient{};
        u_char collision{};
        u_char hits{}; // @todo make it unsigned.
        int hit_reset{}; // @note in seconds
        u_char cloth_type{clothing::none}; /* use clothing:: if you are unsure of the order */
        short rarity{};
        int tick{}; // @note tile tick time e.g. trees, providers, ect.. (measured in seconds)

        std::string info{}; // @note version 22 | item's info

        std::array<u_short, 2> splice{}; // @note version 23 | {seed1, seed2}
    };
    extern std::vector<item> items;

    extern std::vector<u_char> im_data;

    extern void cache_items();

#endif