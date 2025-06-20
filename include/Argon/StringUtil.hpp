#ifndef ARGON_STRINGUTIL_INCLUDE
#define ARGON_STRINGUTIL_INCLUDE

#include <algorithm>
#include <string>

namespace Argon {

template<typename T> requires std::is_integral_v<T>
std::string format_with_commas(const T value) {
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

template<typename T> requires std::is_integral_v<T>
std::string format_with_underscores(const T value) {
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

inline void to_lower(std::string& str) {
    std::ranges::transform(str, str.begin(),
        [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

inline void to_upper(std::string& str) {
    std::ranges::transform(str, str.begin(),
        [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

} // End namespace Argon::StringUtil

#endif // ARGON_STRINGUTIL_INCLUDE