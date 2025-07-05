#include "pch.hpp"
#include "tools/string.hpp"
#include "shouhin.hpp"

#include <map>

std::vector<std::pair<short, shouhin>> shouhin_list{};

void read_shouhin_list()
{
    std::ifstream file("resources/shouhin_list.txt");
    {
        std::string line;
        while (std::getline(file, line)) 
        {
            if (line.empty() || line.contains("#")) continue; // @note checking if line has '#' for comments ^-^
            std::vector<std::string> pipes = readch(std::move(line), '|');
            ::shouhin shouhin{
                .btn = pipes[1],
                .name = pipes[2],
                .rttx = pipes[3],
                .description = pipes[4],
                .tex1 = pipes[5][0],
                .tex2 = pipes[6][0], // @todo
                .cost = stoi(pipes[7])
            };
            std::vector<std::string> tachi = readch(std::move(pipes[8]), ',');
            for (std::string &im : tachi)
            {
                std::vector<std::string> co = readch(std::move(im), ':');
                shouhin.im.emplace_back(stoi(co[0]), stoi(co[1]));
            }
            shouhin_list.emplace_back(stoi(pipes[0]), shouhin);
        }
    }
}