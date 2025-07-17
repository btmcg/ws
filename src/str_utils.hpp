#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ws {

void
ltrim(std::string& s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](std::uint8_t c) { return !std::isspace(c); }));
}

// Function to trim trailing whitespace
void
rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](std::uint8_t c) { return !std::isspace(c); })
                    .base(),
            s.end());
}

void
trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}

std::vector<std::string>
tokenize(std::string const& str)
{
    std::vector<std::string> tokens;
    auto itr = str.begin();

    while (itr != str.end()) {
        // skip leading whitespace
        itr = std::find_if_not(itr, str.end(), [](std::uint8_t c) { return std::isspace(c); });

        // find end of word
        auto word_end
                = std::find_if(itr, str.end(), [](std::uint8_t c) { return std::isspace(c); });

        // if a non-empty word is found, add it to tokens
        if (itr != word_end) {
            tokens.emplace_back(itr, word_end);
        }

        itr = word_end; // move iterator past the current word/whitespace
    }
    return tokens;
}

std::string
to_upper(std::string const& str)
{
    std::string rv = str;
    std::transform(
            rv.begin(), rv.end(), rv.begin(), [](std::uint8_t c) { return std::toupper(c); });
    return rv;
}

std::string
to_lower(std::string const& str)
{
    std::string rv = str;
    std::transform(
            rv.begin(), rv.end(), rv.begin(), [](std::uint8_t c) { return std::tolower(c); });
    return rv;
}

} // namespace ws
