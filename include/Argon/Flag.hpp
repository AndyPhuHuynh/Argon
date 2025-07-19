#ifndef ARGON_FLAG_INCLUDE
#define ARGON_FLAG_INCLUDE

#include <algorithm>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Argon {

struct Flag;
struct FlagPathWithAlias;
struct FlagPath;


struct Flag {
    std::string mainFlag;
    std::vector<std::string> aliases;

    Flag() = default;

    explicit Flag(std::string_view flag);

    [[nodiscard]] auto containsFlag(std::string_view flag) const -> bool;

    [[nodiscard]] auto getString() const -> std::string;

    [[nodiscard]] auto isEmpty() const -> bool;

    auto applyPrefixes(std::string_view shortPrefix, std::string_view longPrefix);

    auto operator<=>(const Flag&) const = default;
};

struct FlagPathWithAlias {
    std::vector<Flag> groupPath;
    Flag flag;

    FlagPathWithAlias() = default;

    explicit FlagPathWithAlias(const FlagPath& flagPath);

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

    FlagPath(std::initializer_list<std::string_view> flags);

    auto operator<=>(const FlagPath&) const = default;

    [[nodiscard]] auto getString() const -> std::string;

    auto extendPath(std::string_view newFlag) -> void;
};

class InvalidFlagPathException : public std::runtime_error {
public:
    explicit InvalidFlagPathException(const FlagPath& flagPath);
    explicit InvalidFlagPathException(std::string_view msg);
};

class IFlag {
protected:
    Flag m_flag;
public:
    IFlag() = default;
    ~IFlag() = default;

    IFlag(const IFlag&) = default;
    IFlag& operator=(const IFlag&) = default;

    IFlag(IFlag&&) = default;
    IFlag& operator=(IFlag&&) = default;

    [[nodiscard]] auto getFlag() const -> const Flag&;
};

template <typename Derived>
class HasFlag : public IFlag{
    auto applySetFlag(std::string_view flag) -> void;
    auto applySetFlag(std::initializer_list<std::string_view> flags) -> void;
public:
    auto operator[](std::string_view flag) & -> Derived&;
    auto operator[](std::string_view flag) && -> Derived;

    auto operator[](std::initializer_list<std::string_view> flags) & -> Derived&;
    auto operator[](std::initializer_list<std::string_view> flags) && -> Derived;
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

inline auto isAlias(const Flag& flag1, const Flag& flag2) -> bool {
    const auto matches = [&](const std::string& alias) {
        return flag2.mainFlag == alias || std::ranges::contains(flag2.aliases, alias);
    };

    if (!matches(flag1.mainFlag)) {
        return false;
    }

    return std::ranges::all_of(flag1.aliases, matches);
}

inline auto isAlias(const FlagPathWithAlias& flagPathWithAlias, const FlagPath& flagPath) -> bool {
    if (flagPathWithAlias.groupPath.size() != flagPath.groupPath.size()) return false;
    for (size_t i = 0; i < flagPath.groupPath.size(); i++) {
        if (!flagPathWithAlias.groupPath[i].containsFlag(flagPath.flag)) return false;
    }
    return flagPathWithAlias.flag.containsFlag(flagPath.flag);
}

inline auto isAlias(const FlagPathWithAlias& flagPath1, const FlagPathWithAlias& flagPath2) -> bool {
    if (flagPath1.groupPath.size() != flagPath2.groupPath.size()) return false;
    for (size_t i = 0; i < flagPath1.groupPath.size(); i++) {
        if (!isAlias(flagPath1.groupPath[i], flagPath2.groupPath[i])) {
            return false;
        }
    }
    return isAlias(flagPath1.flag, flagPath2.flag);
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

inline auto Flag::isEmpty() const -> bool {
    return mainFlag.empty() && aliases.empty();
}

inline FlagPathWithAlias::FlagPathWithAlias(std::vector<Flag> path, Flag flag) : groupPath(std::move(path)), flag(std::move(flag)) {}

inline FlagPathWithAlias::FlagPathWithAlias(const FlagPath& flagPath) {
    for (const auto& path : flagPath.groupPath) {
        groupPath.emplace_back(path);
    }
    flag = Flag(flagPath.flag);
}

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

inline FlagPath::FlagPath(const std::initializer_list<std::string_view> flags) {
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

inline InvalidFlagPathException::InvalidFlagPathException(const std::string_view msg)
    : std::runtime_error(std::string(msg)) {
}

inline auto IFlag::getFlag() const -> const Flag& {
    return m_flag;
}

template<typename Derived>
auto HasFlag<Derived>::applySetFlag(std::string_view flag) -> void {
    if (flag.empty()) {
        throw std::invalid_argument("Flag has to be at least one character long");
    }
    if (m_flag.mainFlag.empty()) {
        m_flag.mainFlag = flag;
    } else {
        m_flag.aliases.emplace_back(flag);
    }
}

template<typename Derived>
auto HasFlag<Derived>::applySetFlag(const std::initializer_list<std::string_view> flags) -> void {
    if (flags.size() <= 0) {
        throw std::invalid_argument("Operator [] expects at least one flag");
    }
    for (const auto& flag : flags) {
        applySetFlag(flag);
    }
}

template <typename Derived>
auto HasFlag<Derived>::operator[](const std::string_view flag) & -> Derived& {
    applySetFlag(flag);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::string_view flag) && -> Derived {
    applySetFlag(flag);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::initializer_list<std::string_view> flags) & -> Derived& {
    applySetFlag(flags);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::initializer_list<std::string_view> flags) && -> Derived {
    applySetFlag(flags);
    return static_cast<Derived&>(*this);
}


} // End namespace Argon

#endif // ARGON_FLAG_INCLUDE