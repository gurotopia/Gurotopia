#include "pch.hpp"
#include <fstream>

#include "database_config.hpp"

::database_config gDatabase_config{};

::database_config load_database_config()
{
    ::database_config config{};
    {
        std::ifstream file("database.cfg");
        if (!file.is_open())
        {
            std::ofstream write("database.cfg");
            write << 
                std::format(
                    /* @note this is read via std::getline() into readch(), pipe-delimited format */
                    "host|{}\n"
                    "port|{}\n"
                    "user|{}\n"
                    "password|{}\n"
                    "schema|{}\n"
                    "RTENDMARKERBS1001",
                    config.host, config.port, config.user, config.password, config.schema
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

            config.host     = pipes[1];
            config.port     = std::stoi(pipes[3]);
            config.user     = pipes[5];
            config.password = pipes[7];
            config.schema   = pipes[9];
        } // @note close file, delete str, pipes
    }
    return config;
}