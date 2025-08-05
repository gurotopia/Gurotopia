#include "pch.hpp"
#include "tools/string.hpp"
#include "shouhin.hpp"

std::vector<std::pair<short, shouhin>> shouhin_tachi{};

void init_shouhin_tachi()
{
    std::ifstream file("resources/store.txt");
    {
        std::string line;
        while (std::getline(file, line)) 
        {
            if (line.empty() || line.starts_with("#")) continue; // @note '#' initiates a comment
            std::vector<std::string> pipes = readch(line, '|');
            ::shouhin shouhin{
                .btn = pipes[1],
                .name = pipes[2],
                .rttx = pipes[3],
                .description = pipes[4],
                .tex1 = pipes[5].front(),
                .tex2 = pipes[6].front(), // @todo
                .cost = stoi(pipes[7])
            };
            std::vector<std::string> tachi = readch(pipes[8], ',');
            for (std::string &im : tachi)
            {
                std::vector<std::string> co = readch(im, ':');
                shouhin.im.emplace_back(stoi(co[0]), stoi(co[1]));
            }
            shouhin_tachi.emplace_back(stoi(pipes[0]), shouhin);
        }
    }
}