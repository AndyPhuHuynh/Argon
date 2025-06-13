#ifndef ARGON_FLAG_INCLUDE
#define ARGON_FLAG_INCLUDE

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Argon {

struct Flag {
    std::string mainFlag;
    std::vector<std::string> aliases;

    Flag() = default;

    explicit Flag(std::string_view flag);

    [[nodiscard]] auto containsFlag(std::string_view flag) const -> bool;

    [[nodiscard]] auto getString() const -> std::string;

    auto applyPrefixes(std::string_view shortPrefix, std::string_view longPrefix);

    auto operator<=>(const Flag&) const = default;
};

struct FlagPathWithAlias {
    std::vector<Flag> groupPath;
    Flag flag;

    FlagPathWithAlias() = default;

    FlagPathWithAlias(std::initializer_list<Flag> flags);

    FlagPathWithAlias(std::vector<Flag> path, Flag flag);

    [[nodiscard]] auto getString() const -> std::string;

    auto operator<=>(const FlagPathWithAlias&) const = default;
};

struct FlagPath {
    std::vector<std::string> groupPath;
    std::string flag;

    FlagPath() = default;

    explicit FlagPath(std::string_view flag);

    FlagPath(std::initializer_list<std::string> flags);

    auto operator<=>(const FlagPath&) const = default;

    [[nodiscard]] auto getString() const -> std::string;

    auto extendPath(std::string_view newFlag) -> void;
};

class InvalidFlagPathException : public std::runtime_error {
public:
    explicit InvalidFlagPathException(const FlagPath& flagPath);
};
} // End namespace Argon

//-------------------------------------------------------Hashes---------------------------------------------------------

template<>
struct std::hash<Argon::Flag> {
    std::size_t operator()(const Argon::Flag& flag) const noexcept {
        std::size_t h = 0;
        constexpr std::hash<std::string> string_hasher;
        for (const auto& alias : flag.aliases) {
            h ^= string_hasher(alias) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= string_hasher(flag.mainFlag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

template<>
struct std::hash<Argon::FlagPathWithAlias> {
    std::size_t operator()(const Argon::FlagPathWithAlias& flagPath) const noexcept {
        std::size_t h = 0;
        constexpr std::hash<std::string> string_hasher;
        for (const auto& [mainFlag, aliases] : flagPath.groupPath) {
            h ^= string_hasher(mainFlag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= string_hasher(flagPath.flag.mainFlag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

template<>
struct std::hash<Argon::FlagPath> {
    std::size_t operator()(const Argon::FlagPath& flagPath) const noexcept {
        std::size_t h = 0;
        constexpr std::hash<std::string> string_hasher;
        for (const auto& flag : flagPath.groupPath) {
            h ^= string_hasher(flag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= string_hasher(flagPath.flag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
inline auto operator==(const FlagPathWithAlias& flagPathWithAlias, const FlagPath& flagPath) {
    if (flagPathWithAlias.groupPath.size() != flagPath.groupPath.size()) return false;
    for (size_t i = 0; i < flagPath.groupPath.size(); i++) {
        if (!flagPathWithAlias.groupPath[i].containsFlag(flagPath.flag)) return false;
    }
    return flagPathWithAlias.flag.containsFlag(flagPath.flag);
}

inline auto operator==(const FlagPath& flagPath, const FlagPathWithAlias& flagPathWithAlias) {
    if (flagPathWithAlias.groupPath.size() != flagPath.groupPath.size()) return false;
    for (size_t i = 0; i < flagPath.groupPath.size(); i++) {
        if (!flagPathWithAlias.groupPath[i].containsFlag(flagPath.flag)) return false;
    }
    return flagPathWithAlias.flag.containsFlag(flagPath.flag);
}

inline auto addPrefixToString(std::string& flag,
    const std::string_view shortPrefix, const std::string_view longPrefix) -> void {
    if (flag.empty()) throw std::invalid_argument("Flag has to be at least one character long");

    if (flag.starts_with(shortPrefix) || flag.starts_with(longPrefix)) return;

    flag.length() == 1 ?
        flag.insert(0, shortPrefix) :
        flag.insert(0, longPrefix);
}
} // End namespace Argon

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {
inline Flag::Flag(const std::string_view flag) : mainFlag(flag) {}

inline auto Flag::containsFlag(const std::string_view flag) const -> bool {
    return mainFlag == flag || std::ranges::contains(aliases, flag);
}

inline auto Flag::getString() const -> std::string {
    std::stringstream ss(mainFlag);
    ss << mainFlag;
    for (auto& alias : aliases) {
        ss << ", " << alias;
    }
    return ss.str();
}

inline auto Flag::applyPrefixes(const std::string_view shortPrefix, const std::string_view longPrefix) {
    addPrefixToString(mainFlag, shortPrefix, longPrefix);
    for (auto& alias : aliases) {
        addPrefixToString(alias, shortPrefix, longPrefix);
    }
}

inline FlagPathWithAlias::FlagPathWithAlias(std::vector<Flag> path, Flag flag) : groupPath(std::move(path)), flag(std::move(flag)) {}

inline FlagPathWithAlias::FlagPathWithAlias(const std::initializer_list<Flag> flags) {
    if (flags.size() == 0) {
        throw std::invalid_argument("FlagPath must contain at least one flag.");
    }

    const auto begin = std::begin(flags);
    const auto end   = std::prev(std::end(flags));

    groupPath.insert(groupPath.end(), begin, end);
    flag = Flag(*end);
}

inline auto FlagPathWithAlias::getString() const -> std::string {
    if (groupPath.empty()) return flag.mainFlag;

    return std::accumulate(std::next(groupPath.begin()), groupPath.end(), groupPath.front().mainFlag, []
        (const std::string& str, const Flag& flag2) -> std::string {
            return str + " > " + flag2.mainFlag;
    }) + " > " + flag.mainFlag;
}


inline FlagPath::FlagPath(const std::string_view flag) : flag(flag) {}

inline FlagPath::FlagPath(const std::initializer_list<std::string> flags) {
    if (flags.size() == 0) {
        throw std::invalid_argument("FlagPath must contain at least one flag.");
    }

    const auto begin = std::begin(flags);
    const auto end   = std::prev(std::end(flags));

    groupPath.insert(groupPath.end(), begin, end);
    flag = *end;
}

inline auto FlagPath::getString() const -> std::string {
    if (groupPath.empty()) return flag;

    return std::accumulate(std::next(groupPath.begin()), groupPath.end(), groupPath.front(), []
        (const std::string& str1, const std::string& str2) -> std::string {
            return str1 + " > " + str2;
    }) + " > " + flag;
}

inline auto FlagPath::extendPath(const std::string_view newFlag) -> void {
    if (flag.empty()) {
        flag = newFlag;
        return;
    }

    groupPath.emplace_back(flag);
    flag = newFlag;
}

inline InvalidFlagPathException::InvalidFlagPathException(const FlagPath& flagPath)
    : std::runtime_error(std::format(
        "Invalid flag path: {}. Check to see if the specified path and templated type are correct.",
        flagPath.getString())) {
}

} // End namespace Argon

#endif // ARGON_FLAG_INCLUDE