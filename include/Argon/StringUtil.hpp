#ifndef ARGON_STRINGUTIL_INCLUDE
#define ARGON_STRINGUTIL_INCLUDE

#include <algorithm>
#include <string>

namespace Argon::detail {

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

inline auto iequals(std::string_view a, std::string_view b) -> bool {
    return a.size() == b.size() &&
       std::ranges::equal(a, b,
           [](const char one, const char two) {
              return std::tolower(static_cast<unsigned char>(one)) ==
                     std::tolower(static_cast<unsigned char>(two));
          });
}


inline auto wrapString(const std::string_view str, const size_t lineLength) -> std::vector<std::string> {
    std::vector<std::string> sections;
    size_t sectionStart = 0;
    while (sectionStart < str.length()) {
        // Skip any leading whitespace
        while (sectionStart < str.length() && str[sectionStart] == ' ') {
            ++sectionStart;
        }
        if (sectionStart >= str.length()) break;

        // Determine the maximum end index
        const size_t end = sectionStart + lineLength;
        if (end >= str.length()) {
            // Add the final section
            sections.emplace_back(str.substr(sectionStart, str.length() - sectionStart));
            break;
        }

        // Find the last space within the range
        size_t breakPoint = str.rfind(' ', end);
        if (breakPoint == std::string::npos || breakPoint <= sectionStart) {
            // No space found, or the space is before sectionStart
            breakPoint = end;
        }

        const size_t length = breakPoint - sectionStart;
        sections.emplace_back(str.substr(sectionStart, length));
        sectionStart = breakPoint;
    }
    return sections;
}

} // End namespace Argon

#endif // ARGON_STRINGUTIL_INCLUDE