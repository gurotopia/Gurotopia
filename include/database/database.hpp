#pragma once

#include <mysql/mysql.h>

extern MYSQL *db;

extern void mysql_connect();

class hStmt
{
public:
    hStmt(const char *query);
   ~hStmt();

    hStmt           (const hStmt&) = delete;
    hStmt& operator=(const hStmt&) = delete;

    MYSQL_STMT *pStmt;
};


extern MYSQL_BIND make_bind_in(const signed &buffer);
extern MYSQL_BIND make_bind_in(const unsigned &buffer);
extern MYSQL_BIND make_bind_in(const long &buffer);
extern MYSQL_BIND make_bind_in(const long long &buffer);
extern MYSQL_BIND make_bind_in(const float &buffer);
extern MYSQL_BIND make_bind_in(const std::string &buffer);
extern MYSQL_BIND make_bind_in(const std::vector<u_char> &buffer);

extern MYSQL_BIND make_bind_out(signed &buffer);
extern MYSQL_BIND make_bind_out(unsigned &buffer);
extern MYSQL_BIND make_bind_out(long &buffer);
extern MYSQL_BIND make_bind_out(long long &buffer);
extern MYSQL_BIND make_bind_out(float &buffer);
extern MYSQL_BIND make_bind_out(std::string &buffer);
