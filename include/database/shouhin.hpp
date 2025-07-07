#pragma once
#ifndef SHOHIN_HPP
#define SHOHIN_HPP

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
    extern std::vector<std::pair<short, shouhin>> shouhin_tachi; // @note {tab, shouhin}

    extern void init_shouhin_tachi();

#endif