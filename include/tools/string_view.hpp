#pragma once
#ifndef STRING_VIEW_HPP
#define STRING_VIEW_HPP

    /*
     @param str the whole string
     @param c the char to search for and split
     @return std::vector<std::string> storing each split off from c found in the str.
    */
    inline std::vector<std::string> readch(const std::string &&str, char c) {
        std::vector<std::string> result;
        result.reserve(std::ranges::count(str, c) + 1);

        for (auto &&part : std::views::split(str, c))
            result.emplace_back(std::ranges::begin(part), std::ranges::end(part));

        return result;
    }

    /*
     @return true if string contains ONLY alpha [a, b, c] or num [1, 2, 3] 
    */
    inline bool alpha(const std::string& str) 
    {
        return std::ranges::all_of(str, [](unsigned char c) { return std::isalnum(c); });
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

    inline std::string base64Decode(std::string_view encoded) {
        static constexpr auto lookup = createLookupTable();
        std::string decoded;
        decoded.reserve((encoded.size() * 3zu) / 4zu);

        int val = 0, valb = -8;
        for (unsigned char c : encoded) 
        {
            int idx = lookup[c];
            if (idx == -1) continue;
            val = (val << 6) | idx;
            valb += 6;
            if (valb >= 0) 
            {
                decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
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

    /* 
     @return '1' (true) || '0' (false) 
    */
    constexpr auto to_char = [](bool b) { return b ? '1' : '0'; };
    
#endif