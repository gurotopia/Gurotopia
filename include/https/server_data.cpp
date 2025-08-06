#include "pch.hpp"
#include "tools/string.hpp"
#include "server_data.hpp"

::_server_data init_server_data()
{
    ::_server_data server_data{};
    {
        std::ifstream file("server_data.php");
        if (!file.is_open())
        {
            std::ofstream write("server_data.php");
            write << 
                std::format(
                    "server|{}\n"
                    "port|{}\n"
                    "type|{}\n"
                    "type2|{}\n"
                    "#maint|{}\n"
                    "loginurl|{}\n"
                    "meta|{}\n"
                    "RTENDMARKERBS1001", 
                    server_data.server, server_data.port, server_data.type, server_data.type2, server_data.maint, server_data.loginurl, server_data.meta
                );
        } // @note close write
        else
        {
            std::vector<std::string> pipes;
            for (std::string line; std::getline(file, line); ) 
            {
                auto pipe_pair = readch(line, '|');
                pipes.insert(pipes.end(), pipe_pair.begin(), pipe_pair.end());
            }

            server_data.server = pipes[1];
            server_data.port = std::stoi(pipes[3]);
            server_data.type = std::stoi(pipes[5]);
            server_data.type2 = std::stoi(pipes[7]);
            server_data.maint = pipes[9];
            server_data.loginurl = pipes[11];
            server_data.meta = pipes[13];
        } // @note delete str, pipes
    } // @note close file
    return server_data;
}