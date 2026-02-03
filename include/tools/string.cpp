#include "pch.hpp"
#include <ranges>

#include "string.hpp"

std::vector<std::string> readch(const std::string &str, char c) 
{
    std::vector<std::string> result;
    result.reserve(std::ranges::count(str, c) + 1);

    for (auto &&part : std::views::split(str, c))
        result.emplace_back(std::ranges::begin(part), std::ranges::end(part));

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
    if (encoded.empty()) return {};

    BIO *bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO *base64 = BIO_new(BIO_f_base64());

    bio = BIO_push(base64, bio);
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);

    std::string decode(encoded.size() * 3/4, '\0'); // @note RFC 4648
    const int bio_read = BIO_read(bio, decode.data(), static_cast<int>(decode.size()));
    BIO_free_all(bio);

    if (bio_read <= 0) return {};
    decode.resize(static_cast<std::size_t>(bio_read));
    return decode;
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
