#pragma once

#include <string>

/* Password hashing via OpenSSL PBKDF2-HMAC-SHA256 (libcrypto, already linked).
 * Cross-platform — no libcrypt/glibc dependency, builds on Linux and Windows.
 * Stored format: pbkdf2_sha256$<iterations>$<salt_b64>$<hash_b64> (~90 chars).
 */
std::string password_hash(const std::string &password);
bool        password_verify(const std::string &password, const std::string &stored);
