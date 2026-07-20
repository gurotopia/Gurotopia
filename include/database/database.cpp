#include "pch.hpp"

#include "database.hpp"
#include "database_config.hpp"

MYSQL *db;

void create_table_if_not_exist()
{
    {
        std::string query = R"(
            CREATE TABLE IF NOT EXISTS peer (
                uid INT AUTO_INCREMENT PRIMARY KEY,
                growid VARCHAR(18) UNIQUE,
                password VARCHAR(128),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )";
        if (mysql_query(db, query.c_str()))
        {
            fprintf(stderr, "%s\n", mysql_error(db));
        }
    } // @note delete query
    {
        std::string query = R"(
            CREATE TABLE IF NOT EXISTS world (
                name VARCHAR(64) NOT NULL PRIMARY KEY,
                blocks BLOB NULL
            );
        )";
        if (mysql_query(db, query.c_str()))
        {
            fprintf(stderr, "%s\n", mysql_error(db));
        }
    } // @note delete query
}

void mysql_connect()
{
    db = mysql_init(NULL);

    if (!mysql_real_connect(db, gDb_config.host.c_str(), gDb_config.user.c_str(), (gDb_config.password.empty()) ? NULL : gDb_config.password.c_str(), NULL, 3306u, NULL, 0u)) 
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    else printf("connected to MariaDB server on %s:%d\n", db->host, db->port);

    mysql_query(db, "CREATE DATABASE IF NOT EXISTS gurotopia");
    mysql_select_db(db, "gurotopia");

    create_table_if_not_exist();
}

/* hStmt */

hStmt::hStmt(const std::string &query)
{
    this->pStmt = mysql_stmt_init(db);
    if (!pStmt) 
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    if (mysql_stmt_prepare(pStmt, query.c_str(), (u_long)query.size()))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
}
hStmt::~hStmt() 
{
    if (mysql_stmt_close(pStmt))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
}

void hStmt::bind_and_execute(MYSQL_BIND *param)
{
    if (mysql_stmt_bind_param(pStmt, param))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    if (mysql_stmt_execute(pStmt))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
}

/* ~ hStmt ~ */


MYSQL_BIND make_bind_in(const signed &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_in(const unsigned &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG, .is_unsigned = true };
}
MYSQL_BIND make_bind_in(const long &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_in(const long long &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONGLONG };
}
MYSQL_BIND make_bind_in(const float &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_FLOAT };
}
MYSQL_BIND make_bind_in(const std::string &buffer)
{
    return { .buffer = (void*)buffer.c_str(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_STRING };
}
MYSQL_BIND make_bind_in(const std::vector<u_char> &buffer)
{
    return { .buffer = (void*)buffer.data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_BLOB };
}

MYSQL_BIND make_bind_out(signed &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_out(unsigned &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG, .is_unsigned = true };
}
MYSQL_BIND make_bind_out(long &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_out(long long &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONGLONG };
}
MYSQL_BIND make_bind_out(float &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_FLOAT };
}
MYSQL_BIND make_bind_out(std::string &buffer)
{
    buffer.resize(1024, '\0');

    return { .buffer = buffer.data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_STRING };
}
MYSQL_BIND make_bind_out(std::vector<u_char> &buffer)
{
    buffer.resize(cord(0, 60)* sizeof(::block));

    return { .buffer = buffer.data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_BLOB };
}
