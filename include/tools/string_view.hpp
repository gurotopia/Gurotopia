#pragma once

/*
 @param str the whole string
 @param c the char to search for and split
 @return std::vector<std::string> storing each split off from c found in the str.
*/
inline/* todo */ std::vector<std::string> readch(const std::string& str, const char& c) 
{
    std::vector<std::string> separations;
    separations.reserve(std::count(str.begin(), str.end(), c) + 1); 
    for (auto&& part : str | std::views::split(c))
        separations.emplace_back(std::string(std::ranges::begin(part), std::ranges::end(part)));
    return separations;
}

/* @return true if string contains ONLY alpha [a, b, c] or digits [1, 2, 3] */
inline/* todo */ bool alpha(const std::string& str) 
{
    for (char c : str)
        if (not std::isalnum(static_cast<unsigned char>(c)))
            return false;
    return true;
}

constexpr std::array<int, 256zu> createLookupTable() noexcept 
{
    std::array<int, 256zu> table{};
    table.fill(-1);
    
    constexpr std::string_view base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    for (std::size_t i = 0zu; i < base64_chars.size(); ++i)
        table[static_cast<unsigned char>(base64_chars[i])] = static_cast<int>(i);

    return table;
}

inline/* todo */ std::string base64Decode(std::string_view encoded) 
{
    constexpr auto lookupTable = createLookupTable();
    std::string decoded;
    decoded.reserve((encoded.size() * 3) / 4);

    int val{};
    int valb{-8};
    for (unsigned char c : encoded) 
    {
        if (int idx = lookupTable[c]; idx != -1) 
        {
            val = (val << 6) + idx;
            valb += 6;
            if (valb >= 0) 
            {
                decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
    }
    return decoded;
}

// @todo downgrade to a int (4 bit)
inline std::size_t fnv1a(const std::string& value) noexcept {
    constexpr std::size_t offset = 14695981039346656037zu;
    constexpr std::size_t prime = 1099511628211zu;

    std::size_t fnv1a = offset;
    for (unsigned char c : value) 
    {
        fnv1a ^= c;
        fnv1a *= prime;
    }
    return fnv1a;
}
