#include "Argon/StringUtil.hpp"

#include <algorithm>
#include <cstdint>

/**
 * @brief Tokenizes a string based on a delimiter
 * @param str The string that will be tokenized
 * @param delimiter The character used as the delimiter
 * @return A vector of the tokens in the tokenized string
 */
std::vector<std::string> Argon::StringUtil::split_string(const std::string& str, const char delimiter) {
    std::vector<std::string> tokens;
    size_t prevIndex = 0;
    const size_t length = str.length();
    for (size_t i = 0; i < length; i++) {
        if (str[i] == delimiter) {
            // Add token only if it's non-empty
            if (prevIndex != i) {
                tokens.push_back(str.substr(prevIndex, i - prevIndex));
            }
            // Skip consecutive delimiters
            while (i < length && str[i] == delimiter) {
                i++;
            }
            prevIndex = i;
        }
    }

    // Add the last token if its non-empty
    if (prevIndex < length) {
        tokens.push_back(str.substr(prevIndex));
    }
    
    return tokens;
}

std::string Argon::StringUtil::format_with_commas(const int64_t value) {
    std::string num = std::to_string(value);

    if (num.length() <= 3) return num;

    size_t insertPosition = num.length() - 3;
    while (insertPosition > 0 && num[(insertPosition) - 1] != '-') {
        num.insert(insertPosition, ",");
        if (insertPosition <= 3) {
            break;
        }
        insertPosition -= 3;
    }
    return num;
}

std::string Argon::StringUtil::format_with_underscores(const int64_t value) {
    std::string num = std::to_string(value);

    if (num.length() <= 3) return num;

    size_t insertPosition = num.length() - 3;
    while (insertPosition > 0 && num[(insertPosition) - 1] != '-') {
        num.insert(insertPosition, "_");
        if (insertPosition <= 3) {
            break;
        }
        insertPosition -= 3;
    }
    return num;
}

void Argon::StringUtil::to_lower(std::string& str) {
    std::ranges::transform(str, str.begin(),
        [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

void Argon::StringUtil::to_upper(std::string& str) {
    std::ranges::transform(str, str.begin(),
        [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
}
