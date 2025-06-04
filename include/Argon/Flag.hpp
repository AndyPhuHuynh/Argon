#ifndef ARGON_FLAG_INCLUDE
#define ARGON_FLAG_INCLUDE

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

    auto operator<=>(const Flag&) const = default;
};

struct FlagPath {
    std::vector<Flag> groupPath;
    Flag flag;

    FlagPath() = default;

    explicit FlagPath(std::string_view flag);

    FlagPath(std::initializer_list<std::string> flags);

    FlagPath(std::initializer_list<Flag> flags);

    FlagPath(std::vector<Flag> path, Flag flag);

    [[nodiscard]] auto getString() const -> std::string;

    auto operator<=>(const FlagPath&) const = default;
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
struct std::hash<Argon::FlagPath> {
    std::size_t operator()(const Argon::FlagPath& flagPath) const noexcept {
        std::size_t h = 0;
        constexpr std::hash<std::string> string_hasher;
        for (const auto& [mainFlag, aliases] : flagPath.groupPath) {
            h ^= string_hasher(mainFlag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= string_hasher(flagPath.flag.mainFlag) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {
inline Flag::Flag(const std::string_view flag) : mainFlag(flag) {}

inline auto Flag::containsFlag(const std::string_view flag) const -> bool {
    return mainFlag == flag || std::ranges::contains(aliases, flag);
}

inline FlagPath::FlagPath(const std::string_view flag) : flag(flag) {}

inline FlagPath::FlagPath(const std::initializer_list<std::string> flags) {
    if (flags.size() == 0) {
        throw std::invalid_argument("FlagPath must contain at least one flag.");
    }

    const auto begin = std::begin(flags);
    const auto end   = std::prev(std::end(flags));

    for (auto it = begin; it != end; ++it) {
        groupPath.emplace_back(*it);
    }
    flag = Flag(*end);
}

inline FlagPath::FlagPath(const std::initializer_list<Flag> flags) {
    if (flags.size() == 0) {
        throw std::invalid_argument("FlagPath must contain at least one flag.");
    }

    const auto begin = std::begin(flags);
    const auto end   = std::prev(std::end(flags));

    groupPath.insert(groupPath.end(), begin, end);
    flag = Flag(*end);
}

inline FlagPath::FlagPath(std::vector<Flag> path, Flag flag) : groupPath(std::move(path)), flag(std::move(flag)) {}

inline auto FlagPath::getString() const -> std::string {
    if (groupPath.empty()) return flag.mainFlag;

    return std::accumulate(std::next(groupPath.begin()), groupPath.end(), groupPath.front().mainFlag, []
        (const std::string& str, const Flag& flag2) -> std::string {
            return str + " > " + flag2.mainFlag;
    }) + " > " + flag.mainFlag;
}

inline InvalidFlagPathException::InvalidFlagPathException(const FlagPath& flagPath)
    : std::runtime_error(std::format(
        "Invalid flag path: {}. Check to see if the specified path and templated type are correct.",
        flagPath.getString())) {
}

} // End namespace Argon

#endif // ARGON_FLAG_INCLUDE