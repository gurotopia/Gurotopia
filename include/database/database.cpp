#include "pch.hpp"

#include "database.hpp"
#include "database_config.hpp"

MYSQL *db;

void create_table_if_not_exist()
{
    std::string query_peer = 
        "CREATE TABLE IF NOT EXISTS peer ("
            "uid INT AUTO_INCREMENT PRIMARY KEY,"
            "growid VARCHAR(18) UNIQUE,"
            "password VARCHAR(18),"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "inventory BLOB"
        ")";
    std::string query_world = 
        "CREATE TABLE IF NOT EXISTS world ("
            "name VARCHAR(24) NOT NULL PRIMARY KEY,"
            "owner INT DEFAULT 0,"
            "blocks BLOB,"
            "objects BLOB"
        ")";
    if (mysql_query(db, query_peer.c_str()) || mysql_query(db, query_world.c_str()))
    {
        std::fprintf(stderr, "%s\n", mysql_error(db));
    }

    /* things added later: created here so nobody has to run SQL by hand (errors are ignored, they just mean "already there") */
    static constexpr const char *upgrades[]{
        "ALTER TABLE world ADD COLUMN IF NOT EXISTS access VARCHAR(400) NOT NULL DEFAULT ''",
        "ALTER TABLE world ADD COLUMN IF NOT EXISTS treasure VARCHAR(2000) NOT NULL DEFAULT ''",
        "ALTER TABLE world ADD COLUMN IF NOT EXISTS weather INT NOT NULL DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS curse_until INT NOT NULL DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS ban_until INT NOT NULL DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS god INT NOT NULL DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS renamed_at INT NOT NULL DEFAULT 0",
        "CREATE TABLE IF NOT EXISTS renames (old_name VARCHAR(64) PRIMARY KEY, new_name VARCHAR(64) NOT NULL)",
        "CREATE TABLE IF NOT EXISTS friends (uid INT NOT NULL, friend_uid INT NOT NULL, PRIMARY KEY (uid, friend_uid))",
    };
    for (const char *sql : upgrades) mysql_query(db, sql);
}

void mysql_connect()
{
    db = mysql_init(NULL);

    if (mysql_real_connect(db, gDb_config.host.c_str(), gDb_config.user.c_str(), gDb_config.passwd.c_str(), NULL, 3306u, NULL, 0ul) == NULL) 
    {
        std::fprintf(stderr, "[MariaDB] %s\n", mysql_error(db));
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
        std::fprintf(stderr, "%s\n", mysql_error(db));
    }
    if (mysql_stmt_prepare(pStmt, query.c_str(), (u_long)query.size()))
    {
        std::fprintf(stderr, "%s\n", mysql_error(db));
    }
}
hStmt::~hStmt() 
{
    if (mysql_stmt_close(pStmt))
    {
        std::fprintf(stderr, "%s\n", mysql_error(db));
    }
}

void hStmt::bind_param(MYSQL_BIND *param)
{
    if (mysql_stmt_bind_param(pStmt, param)) log_err();
}
void hStmt::execute()
{
    if (mysql_stmt_execute(pStmt)) log_err();
}
void hStmt::fetch()
{
    int value = mysql_stmt_fetch(pStmt);
    if (value == 1 || value == MYSQL_DATA_TRUNCATED) log_err();
    else if (value == 0 || value == MYSQL_NO_DATA) /*@todo do something later...*/; // @note success
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
MYSQL_BIND make_bind_in(const ::blob &buffer)
{
    return { .buffer = (void*)buffer.data().data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_BLOB };
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
MYSQL_BIND make_bind_out(::blob &buffer)
{
    buffer.resize(cord(0, 60)* sizeof(::block));

    return { .buffer = buffer.data().data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_BLOB };
}
