#pragma once

#include <string>

/* bcrypt password hashing via libcrypt's crypt() + OpenSSL RAND_bytes.
 * Uses $2b$ prefix with cost factor 12 (matching GTPS leak sources).
 * Hash output is always 60 characters.
 */
std::string bcrypt_hash(const std::string &password);
bool        bcrypt_verify(const std::string &password, const std::string &hash);