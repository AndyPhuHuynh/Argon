#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Argon::StringUtil {
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    
    std::string format_with_commas(int64_t value);

    std::string format_with_underscores(int64_t value);

    void to_lower(std::string& str);

    void to_upper(std::string& str);
}

#include "StringUtil.tpp"