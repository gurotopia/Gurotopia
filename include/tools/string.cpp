#include "pch.hpp"

#include "string.hpp"

#include <cctype> // @include std::isalnum
#include <sstream>

std::vector<std::string> readch(const std::string &str, char deli) 
{
    std::vector<std::string> result;
    
    std::istringstream iss;
    iss.str(str);
    for (std::string line; std::getline(iss, line, deli); ) 
    {
        result.push_back(line);
    }
    return result;
}

std::string join(const std::vector<std::string>& range, const std::string& del) 
{
    std::string result;
    for (size_t i = 0; i < range.size(); ++i) 
    {
        result += range[i];
        if (i < range.size() - 1) result += del;
    }
    return result;
}

bool isalnum(const std::string &str) 
{
    for (char c : str)
    {
        if (!std::isalnum((u_char)c)) return false;
    }
    return true;
}

#include <openssl/bio.h>
#include <openssl/evp.h> // @include BIO_f_base64()

std::string base64_decode(const std::string& encoded)
{
    if (encoded.empty()) return {};

    BIO *bio = BIO_new_mem_buf(encoded.data(), (int)(encoded.size()));
    BIO *base64 = BIO_new(BIO_f_base64());

    bio = BIO_push(base64, bio);
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);

    std::string decode(encoded.size() * 3/4, '\0'); // @note RFC 4648
    const int bio_read = BIO_read(bio, decode.data(), (int)(decode.size()));
    BIO_free_all(bio);

    if (bio_read <= 0) return {};
    decode.resize(static_cast<std::size_t>(bio_read));
    return decode;
}
