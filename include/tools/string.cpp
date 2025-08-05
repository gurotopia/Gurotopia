#include "pch.hpp"
#include "tools/string.hpp"

std::vector<std::string> readch(const std::string &str, char c) 
{
    std::vector<std::string> result;
    result.reserve(std::ranges::count(str, c) + 1);

    for (auto &&part : std::views::split(str, c))
        result.emplace_back(std::ranges::begin(part), std::ranges::end(part));

    return result;
}

bool alpha(const std::string& str) 
{
    return !str.empty() && std::ranges::all_of(str, ::isalpha);
}

bool number(const std::string& str) 
{
    return !str.empty() && std::ranges::all_of(str, ::isdigit);
}

bool alnum(const std::string& str) 
{
    return !str.empty() && std::ranges::all_of(str, ::isalnum);
}

#include <openssl/bio.h>
#include <openssl/evp.h>

std::string base64_decode(const std::string& encoded) 
{
    BIO *bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO *base64 = BIO_new(BIO_f_base64());

    bio = BIO_push(base64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    std::vector<char> buf(encoded.size());
    int bio_read = BIO_read(bio, buf.data(), static_cast<int>(buf.size()));
    BIO_free_all(bio);

    return std::string{ buf.data(), static_cast<size_t>(bio_read) };
}

std::size_t fnv1a(const std::string& value) noexcept {
    constexpr std::size_t offset = 14695981039346656037zu;
    constexpr std::size_t prime = 1099511628211zu;

    std::size_t fnv1a = offset;
    for (u_char c : value) 
    {
        fnv1a ^= c;
        fnv1a *= prime;
    }
    return fnv1a;
}