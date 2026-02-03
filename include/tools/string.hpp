#pragma once
#ifndef TOOLS_STRING_HPP
#define TOOLS_STRING_HPP

    /*
        @param str the whole string
        @param c the char to search for and split
        @return std::vector<std::string> storing each split off from c found in the str.
    */
    extern std::vector<std::string> readch(const std::string &str, char c);

    extern std::string join(const std::vector<std::string>& range, const std::string& del);

    /*
        @return true if string contains ONLY alphabet [a, b, c] or number [1, 2, 3] 
    */
    extern bool alpha(const std::string& str);

    /*
        @return true if string contains ONLY number [1, 2, 3] 
    */
    extern bool number(const std::string& str);

    /*
        @return true if string contains ONLY numbers or alphabet [1, 2, 3, a, b, c]
    */
    extern bool alnum(const std::string& str);

    extern std::string base64_decode(const std::string& encoded);

    // @todo downgrade to a int (4 bit)
    extern std::size_t fnv1a(const std::string& value) noexcept;

    /* 
        @return '1' (true) || '0' (false) 
    */
    inline constexpr auto to_char = [](bool b) { return b ? '1' : '0'; };


#endif