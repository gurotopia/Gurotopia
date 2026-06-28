#include "pch.hpp"
#include <crypt.h>
#include <openssl/rand.h>

#include "crypt.hpp"

static const char* bcrypt_b64 =
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

std::string bcrypt_hash(const std::string &password)
{
    // generate salt: $2b$12$<22-char-random>
    unsigned char rnd[16];
    if (!RAND_bytes(rnd, sizeof(rnd)))
    {
        fprintf(stderr, "[bcrypt_hash] RAND_bytes failed\n");
        return "";
    }

    char salt[30];
    salt[0] = '$'; salt[1] = '2'; salt[2] = 'b';
    salt[3] = '$'; salt[4] = '1'; salt[5] = '2'; // cost 2^12
    salt[6] = '$';

    for (int i = 0; i < 22; ++i)
        salt[7 + i] = bcrypt_b64[rnd[i] % 64];
    salt[29] = '\0';

    char *result = crypt(password.c_str(), salt);
    if (!result || result[0] == '*')
    {
        fprintf(stderr, "[bcrypt_hash] crypt() failed\n");
        return "";
    }

    return std::string(result); // always 60 chars
}

bool bcrypt_verify(const std::string &password, const std::string &hash)
{
    char *result = crypt(password.c_str(), hash.c_str());
    return result && (std::string(result) == hash);
}