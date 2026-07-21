#pragma once

#include <mysql/mysql.h>

extern MYSQL *db;

extern void mysql_connect();

class hStmt
{
public:
    hStmt(const std::string &query);
   ~hStmt();

    hStmt           (const hStmt&) = delete;
    hStmt& operator=(const hStmt&) = delete;

    void bind_and_execute(MYSQL_BIND *param);

    MYSQL_STMT *pStmt;
};

struct blob
{
    /* @note template vibes */
    void f32(float val) { 
        mData.resize(i + sizeof(float));
        memcpy(mData.data() + i, &val, sizeof(float)); i += sizeof(float); 
    }
    void i32(int val)   { 
        mData.resize(i + sizeof(int));
        memcpy(mData.data() + i, &val, sizeof(int));   i += sizeof(int); 
    }
    void i16(short val) {
        mData.resize(i + sizeof(short));
        memcpy(mData.data() + i, &val, sizeof(short)); i += sizeof(short); 
    }
    void u8(u_char val) { 
        mData.resize(i + sizeof(u_char));
        mData[i++] = val; 
     }

    const std::vector<u_char> &data() const noexcept { 
        return mData; 
    }

private:
    std::vector<u_char> mData;
    int i{};
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
extern MYSQL_BIND make_bind_out(std::vector<u_char> &buffer);
