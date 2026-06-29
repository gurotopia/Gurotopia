#include "pch.hpp"
#include <fstream>

#include "database_config.hpp"

::database_config gDb_config{};

::database_config load_database_config()
{
    ::database_config db_config{};
    {
        std::ifstream file("database.cfg");
        if (!file.is_open())
        {
            std::ofstream write("database.cfg");
            write << 
                std::format(
                    /* @note this is read via std::getline() into readch(), pipe-delimited format */
                    "host|{}\n"
                    "user|{}\n"
                    "password|{}\n",
                    db_config.host, db_config.user, db_config.password
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

            db_config.host     = pipes[1];
            db_config.user     = pipes[3];
            db_config.password = pipes[5];
        } // @note delete pipes
    } // @note close file
    return db_config;
}