#pragma once

class database_config 
{
public:
    std::string host{"127.0.0.1"};
    std::string user{"root"};
    std::string password{""};
};

extern ::database_config gDb_config; // @note db for short, "database" looked long and ugly.

/*
    @return database_config read from 'database.cfg'.
    if the file does not exist, it will be created with default values.
*/
extern ::database_config load_database_config();