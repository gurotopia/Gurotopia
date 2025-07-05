#pragma once
#ifndef SHOHIN_HPP
#define SHOHIN_HPP

    #include <map>

    class shouhin
    {
    public:
        std::string btn{};
        std::string name{};
        std::string rttx{};
        std::string description{};
        char tex1{};
        char tex2{}; // @todo may have to change variable type...
        int cost{};

        std::vector<std::pair<short, short>> im{}; // @note {id, amount}
    }; 
    extern std::vector<std::pair<short, shouhin>> shouhin_list; // @note {tab, shouhin}

    extern void read_shouhin_list();

#endif