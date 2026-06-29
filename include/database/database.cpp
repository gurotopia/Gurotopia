#include "pch.hpp"

#include "database.hpp"
#include "database_config.hpp"

MYSQL *db;

void create_table_if_not_exist()
{
    const char* query = R"(
        CREATE TABLE IF NOT EXISTS peer (
            uid INT AUTO_INCREMENT PRIMARY KEY,
            growid VARCHAR(18) UNIQUE,
            password VARCHAR(64),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            gems INT DEFAULT 0,
            level SMALLINT UNSIGNED DEFAULT 1,
            xp SMALLINT UNSIGNED DEFAULT 0,
            role TINYINT UNSIGNED DEFAULT 0,
            skin_color INT UNSIGNED DEFAULT 2527912447,
            hair_color INT DEFAULT -1,
            slot_size SMALLINT DEFAULT 16,
            clothing TEXT,
            inventory TEXT
        );
    )";

    if (mysql_query(db, query))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }

    // migrations for existing tables
    auto migrate = [](const char *alter_stmt) {
        if (mysql_query(db, alter_stmt))
        {
            if (mysql_errno(db) == 1054) return; // unknown column (fresh table)
            // 1060 = duplicate column (already migrated), 1064 = syntax (harmless)
            if (mysql_errno(db) != 1060 && mysql_errno(db) != 1064)
                fprintf(stderr, "[migrate] %s: %s\n", alter_stmt, mysql_error(db));
        }
    };

    migrate("ALTER TABLE peer MODIFY COLUMN password VARCHAR(64)");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS gems INT DEFAULT 0");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS level SMALLINT UNSIGNED DEFAULT 1");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS xp SMALLINT UNSIGNED DEFAULT 0");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS role TINYINT UNSIGNED DEFAULT 0");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS skin_color INT UNSIGNED DEFAULT 2527912447");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS hair_color INT DEFAULT -1");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS slot_size SMALLINT DEFAULT 16");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS clothing TEXT");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS inventory TEXT");
    migrate("ALTER TABLE peer ADD COLUMN IF NOT EXISTS last_daily TIMESTAMP NULL DEFAULT NULL");
}

void mysql_connect()
{
    db = mysql_init(NULL);

    my_bool reconnect = 1;
    mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(db, gDatabase_config.host.c_str(), gDatabase_config.user.c_str(), gDatabase_config.password.empty() ? NULL : gDatabase_config.password.c_str(), NULL, gDatabase_config.port, NULL, 0)) 
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    else printf("connected to SQL server on %s:%d\n", db->host, db->port);

    mysql_query(db, ("CREATE DATABASE IF NOT EXISTS " + gDatabase_config.schema).c_str());
    mysql_select_db(db, gDatabase_config.schema.c_str());

    create_table_if_not_exist();
}

hStmt::hStmt(const char *query)
{
    this->pStmt = mysql_stmt_init(db);
    if (!pStmt) 
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    if (mysql_stmt_prepare(pStmt, query, (u_long)strlen(query)))
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
