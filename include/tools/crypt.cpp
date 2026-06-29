#include "pch.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "crypt.hpp"

/* PBKDF2-HMAC-SHA256 via OpenSSL (libcrypto, already linked).
 * Cross-platform: no libcrypt / glibc crypt() dependency, so it
 * builds on both Linux and Windows (MSYS2 UCRT64).
 *
 * Stored format:  pbkdf2_sha256$<iterations>$<salt_b64>$<hash_b64>
 */

static constexpr int    PBKDF2_ITERATIONS = 600000; // OWASP 2023 baseline
static constexpr size_t SALT_LEN          = 16;
static constexpr size_t HASH_LEN          = 32;     // SHA-256 output

/* base64 encode without newlines */
static std::string b64_encode(const unsigned char *data, size_t len)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        unsigned int n = data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];

        out.push_back(tbl[(n >> 18) & 63]);
        out.push_back(tbl[(n >> 12) & 63]);
        out.push_back(i + 1 < len ? tbl[(n >> 6) & 63] : '=');
        out.push_back(i + 2 < len ? tbl[n & 63] : '=');
    }
    return out;
}

/* derive a key from password + salt using PBKDF2-HMAC-SHA256 */
static bool pbkdf2(const std::string &password,
                   const unsigned char *salt, size_t salt_len,
                   int iterations,
                   unsigned char *out, size_t out_len)
{
    return PKCS5_PBKDF2_HMAC(
        password.c_str(), (int)password.size(),
        salt, (int)salt_len,
        iterations,
        EVP_sha256(),
        (int)out_len, out) == 1;
}

std::string password_hash(const std::string &password)
{
    unsigned char salt[SALT_LEN];
    if (!RAND_bytes(salt, sizeof(salt)))
    {
        fprintf(stderr, "[password_hash] RAND_bytes failed\n");
        return "";
    }

    unsigned char hash[HASH_LEN];
    if (!pbkdf2(password, salt, sizeof(salt), PBKDF2_ITERATIONS, hash, sizeof(hash)))
    {
        fprintf(stderr, "[password_hash] PKCS5_PBKDF2_HMAC failed\n");
        return "";
    }

    return std::format("pbkdf2_sha256${}${}${}",
        PBKDF2_ITERATIONS,
        b64_encode(salt, sizeof(salt)),
        b64_encode(hash, sizeof(hash)));
}

bool password_verify(const std::string &password, const std::string &stored)
{
    // parse pbkdf2_sha256$<iter>$<salt_b64>$<hash_b64>
    auto parts = readch(stored, '$');
    if (parts.size() != 4 || parts[0] != "pbkdf2_sha256")
        return false;

    int iterations = std::atoi(parts[1].c_str());
    if (iterations <= 0)
        return false;

    // re-derive with the same salt and compare against the stored hash
    const std::string &salt_b64 = parts[2];
    const std::string &hash_b64 = parts[3];

    // decode salt by re-encoding our derived value instead: simplest is to
    // base64-decode the salt. Implement a tiny decoder inline.
    static const auto b64_val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    auto b64_decode = [](const std::string &in) {
        std::string out;
        int val = 0, bits = -8;
        for (char c : in)
        {
            if (c == '=') break;
            int d = b64_val(c);
            if (d < 0) continue;
            val = (val << 6) | d;
            bits += 6;
            if (bits >= 0)
            {
                out.push_back(char((val >> bits) & 0xFF));
                bits -= 8;
            }
        }
        return out;
    };

    std::string salt = b64_decode(salt_b64);
    if (salt.empty())
        return false;

    unsigned char hash[HASH_LEN];
    if (!pbkdf2(password,
                reinterpret_cast<const unsigned char*>(salt.data()), salt.size(),
                iterations, hash, sizeof(hash)))
        return false;

    std::string computed = b64_encode(hash, sizeof(hash));

    // constant-time compare
    if (computed.size() != hash_b64.size())
        return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < computed.size(); ++i)
        diff |= (unsigned char)(computed[i] ^ hash_b64[i]);
    return diff == 0;
}
