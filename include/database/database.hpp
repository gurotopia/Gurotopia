#pragma once

#include <mysql/mysql.h>

#include "tools/terminal.hpp"

class mysql_gateway
{
public:
    mysql_gateway() { mysql = mysql_init(NULL); }
    ~mysql_gateway() { mysql_close(mysql); }

    void create_table_if_not_exist();
    bool column_found(const std::string &table, const std::string &column);
    int alter_table(const std::string &table, const std::string &column);
    void integrity();

    void mysql_connect();
    void load_login();

    MYSQL *mysql{nullptr};
private:
    std::string host{"127.0.0.1"};
    std::string user{"root"};
    std::string passwd{""};
};
extern ::mysql_gateway db;

class hStmt
{
public:
    hStmt(const std::string &query);
   ~hStmt();

    hStmt           (const hStmt &) = delete;
    hStmt &operator=(const hStmt &) = delete;

    void bind_param(MYSQL_BIND *param);
    void execute();
    void fetch();

    /* https://dev.mysql.com/doc/mysql-errors/8.4/en/client-error-reference.html */
    void log_stmt_error();

    MYSQL_STMT *pStmt;
};

#include <cstring> // linux

struct blob
{
    /* @note template vibes */
    void f32(float val) {
        int size = mData.size();
        mData.resize(size + sizeof(float));
        memcpy(mData.data() + size, &val, sizeof(float));
    }
    void i32(int val)   { 
        int size = mData.size();
        mData.resize(size + sizeof(int));
        memcpy(mData.data() + size, &val, sizeof(int));
    }
    void u32(u_int val)   { 
        int size = mData.size();
        mData.resize(size + sizeof(u_int));
        memcpy(mData.data() + size, &val, sizeof(u_int));
    }
    void i16(short val) {
        int size = mData.size();
        mData.resize(size + sizeof(short));
        memcpy(mData.data() + size, &val, sizeof(short));
    }
    void u8(u_char val) {
        mData.push_back(val);
    }
    
    bool read_f32(float &val, int &pos)
    {
        if (pos + (int)sizeof(float) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(float));
        pos += sizeof(float);
        return true;
    }
    bool read_i32(int &val, int &pos)
    {
        if (pos + (int)sizeof(int) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(int));
        pos += sizeof(int);
        return true;
    }
    bool read_u32(u_int &val, int &pos)
    {
        if (pos + (int)sizeof(u_int) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(u_int));
        pos += sizeof(u_int);
        return true;
    }
    bool read_i16(short &val, int &pos)
    {
        if (pos + (int)sizeof(short) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(short));
        pos += sizeof(short);
        return true;
    }
    bool read_u16(u_short &val, int &pos)
    {
        if (pos + (int)sizeof(u_short) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(u_short));
        pos += sizeof(u_short);
        return true;
    }
    bool read_i8(char &val, int &pos)
    {
        if (pos + (int)sizeof(char) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(char));
        pos += sizeof(char);
        return true;
    }
    bool read_u8(u_char &val, int &pos)
    {
        if (pos + (int)sizeof(u_char) > (int)mData.size()) return false;
        memcpy(&val, mData.data()+pos, sizeof(u_char));
        pos += sizeof(u_char);
        return true;
    }


    void push_back(const blob &blob) 
    { 
        const auto &data = blob.data();
        
        mData.insert(mData.end(), data.cbegin(), data.cend());
    }
    [[nodiscard]] const std::vector<u_char> &data() const noexcept { 
        return mData; 
    }
    [[nodiscard]] std::vector<u_char> &data() noexcept {
        return mData;
    }
    [[nodiscard]] constexpr std::size_t size() const noexcept { 
        return mData.size(); 
    }
    constexpr void resize(std::size_t __new_size)
    {
        mData.resize(__new_size);
    }

private:
    std::vector<u_char> mData;
};

extern MYSQL_BIND make_bind_in(const signed &buffer);
extern MYSQL_BIND make_bind_in(const unsigned &buffer);
extern MYSQL_BIND make_bind_in(const long &buffer);
extern MYSQL_BIND make_bind_in(const long long &buffer);
extern MYSQL_BIND make_bind_in(const float &buffer);
extern MYSQL_BIND make_bind_in(const std::string &buffer);
extern MYSQL_BIND make_bind_in(const std::vector<u_char> &buffer);
extern MYSQL_BIND make_bind_in(const ::blob &buffer);

extern MYSQL_BIND make_bind_out(signed &buffer);
extern MYSQL_BIND make_bind_out(unsigned &buffer);
extern MYSQL_BIND make_bind_out(long &buffer);
extern MYSQL_BIND make_bind_out(long long &buffer);
extern MYSQL_BIND make_bind_out(float &buffer);
extern MYSQL_BIND make_bind_out(std::string &buffer);
extern MYSQL_BIND make_bind_out(std::vector<u_char> &buffer);
extern MYSQL_BIND make_bind_out(::blob &buffer);
