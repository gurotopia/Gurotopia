#pragma once

#include <mysql/mysql.h>

extern MYSQL *db;

extern void mysql_connect();

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
    void log_err() { std::fprintf(stderr, "[MariaDB] %s\n", mysql_error(db)); }

    MYSQL_STMT *pStmt;
};

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
    bool read_string(std::string &val, int &pos)
    {
        short len{};
        if (!read_i16(len, pos) || len < 0 || pos + len > (int)mData.size()) return false;

        val.resize(len);
        memcpy(val.data(), mData.data() + pos, len);
        pos += len;
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
    void u8(u_char val) {
        mData.push_back(val);
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
