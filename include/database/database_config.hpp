#pragma once

class database_config 
{
public:
    std::string host{"127.0.0.1"};
    int port{3306};
    std::string user{"root"};
    std::string password{""};
    std::string schema{"gurotopia"};
};

extern ::database_config gDatabase_config;

/*
    @return database_config read from 'database.cfg'.
    if the file does not exist, it will be created with default values.
*/
extern ::database_config load_database_config();