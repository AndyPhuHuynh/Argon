#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <memory>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "Traits.hpp"

namespace Argon {
    class IOption;
    using OptionPtr = std::variant<IOption*, std::unique_ptr<IOption>>;

    struct FlagPath {
        std::vector<std::string> groupPath;
        std::string flag;

        FlagPath() = default;

        explicit FlagPath(std::string_view flag);

        FlagPath(std::initializer_list<std::string> flags);

        [[nodiscard]] auto getString() const -> std::string;

        auto operator<=>(const FlagPath&) const = default;
    };

    class InvalidFlagPathException : public std::runtime_error {
    public:
        explicit InvalidFlagPathException(const FlagPath& flagPath);
    };

    class Context {
        std::vector<OptionPtr> m_options;
        std::vector<std::string> m_flags;
        std::string m_name;
        Context *m_parent = nullptr;
    public:
        Context() = default;
        explicit Context(std::string name);
        Context(const Context&);
        auto operator=(const Context&) -> Context&;
        
        auto addOption(IOption& option) -> void;

        auto addOption(IOption&& option) -> void;

        auto getOption(const std::string& flag) -> IOption*;

        auto getOption(const FlagPath& flagPath) -> IOption*;

        template <typename T>
        auto getOptionDynamic(const std::string& flag) -> T*;

        auto setName(const std::string& name) -> void;

        [[nodiscard]] auto getPath() const -> std::string;

        [[nodiscard]] auto containsLocalFlag(const std::string& flag) const -> bool;

        template<typename ValueType>
        auto getValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

    private:
        auto resolveFlagGroup(const FlagPath& flagPath) -> Context&;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
    inline auto getRawPointer(const OptionPtr& optPtr) -> IOption* {
        return std::visit([]<typename T>(const T& opt) -> IOption* {
            if constexpr (std::is_same_v<T, IOption*>) {
                return opt;
            } else if constexpr (std::is_same_v<T, std::unique_ptr<IOption>>) {
                return opt.get();
            } else {
                static_assert(always_false<T>, "Unhandled type in getRawPointer");
                return nullptr;
            }
        }, optPtr);
    }
}

//-------------------------------------------------------Hashes---------------------------------------------------------

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

//---------------------------------------------------Implementations----------------------------------------------------

#include <algorithm>

#include "Option.hpp" // NOLINT (misc-unused-include)
#include "MultiOption.hpp"

namespace Argon {
inline FlagPath::FlagPath(const std::string_view flag) : flag(flag) {}

inline FlagPath::FlagPath(const std::initializer_list<std::string> flags) {
    if (flags.size() == 0) {
        throw std::invalid_argument("FlagPath must contain at least one flag.");
    }

    const auto begin = std::begin(flags);
    const auto end   = std::prev(std::end(flags));

    groupPath.insert(std::end(groupPath), begin, end);
    flag = *end;
}

inline auto FlagPath::getString() const -> std::string {
    if (groupPath.empty()) return flag;

    return std::accumulate(std::next(groupPath.begin()), groupPath.end(), groupPath.front(), []
        (const std::string& str1, const std::string& str2) -> std::string {
            return str1 + " > " + str2;
    }) + " > " + flag;
}

inline InvalidFlagPathException::InvalidFlagPathException(const FlagPath& flagPath)
    : std::runtime_error(std::format(
        "Invalid flag path: {}. Check to see if the specified path and templated type are correct.",
        flagPath.getString())) {
}

inline Context::Context(std::string name) : m_name(std::move(name)) {}

inline Context::Context(const Context& other) {
    for (const auto& option : other.m_options) {
        std::visit([&]<typename T>(const T& opt) -> void {
            if constexpr (std::is_same_v<T, IOption*>) {
                m_options.emplace_back(opt);
            } else {
                m_options.emplace_back(opt->clone());
            }
        }, option);

        if (auto *optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(m_options.back()))) {
            optionGroup->getContext().m_parent = this;
        }
    }
    m_flags = other.m_flags;
    m_name = other.m_name;
    m_parent = other.m_parent;
}

inline auto Context::operator=(const Context& other) -> Context& {
    if (this == &other) {
        return *this;
    }

    m_options.clear();
    for (const auto& option : other.m_options) {
        std::visit([&]<typename T>(const T& opt) -> void {
            if constexpr (std::is_same_v<T, IOption*>) {
                m_options.emplace_back(opt);
            } else {
                m_options.emplace_back(opt->clone());
            }
        }, option);

        if (auto *optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(m_options.back()))) {
            optionGroup->getContext().m_parent = this;
        }
    }
    m_flags = other.m_flags;
    m_name = other.m_name;
    m_parent = other.m_parent;
    return *this;
}

inline auto Context::addOption(IOption& option) -> void {
    m_flags.insert(m_flags.end(), option.getFlags().begin(), option.getFlags().end());
    m_options.emplace_back(&option);
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(&option)) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Context::addOption(IOption&& option) -> void {
    m_flags.insert(m_flags.end(), option.getFlags().begin(), option.getFlags().end());
    const auto& newOpt = m_options.emplace_back(option.clone());
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(newOpt))) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Context::getOption(const std::string& flag) -> IOption * {
    const auto it = std::ranges::find_if(m_options, [&flag](const OptionPtr& option) {
        return std::visit([&flag]<typename T>(const T& opt) -> bool {
            return std::ranges::contains(opt->getFlags(), flag);
        }, option);
    });

    return it == m_options.end() ? nullptr : getRawPointer(*it);
}

inline auto Context::getOption(const FlagPath& flagPath) -> IOption * {
    auto& context = resolveFlagGroup(flagPath);
    return context.getOption(flagPath.flag);
}

template <typename T>
auto Context::getOptionDynamic(const std::string& flag) -> T * {
    return dynamic_cast<T*>(getOption(flag));
}

template<typename ValueType>
auto Context::getValue(const FlagPath& flagPath) -> const ValueType& {
    auto& context = resolveFlagGroup(flagPath);
    const auto opt = context.getOptionDynamic<Option<ValueType>>(flagPath.flag);
    if (opt == nullptr) {
        throw InvalidFlagPathException(flagPath);
    }
    return opt->getValue();
}

template<typename Container>
auto Context::getMultiValue(const FlagPath& flagPath) -> const Container& {
    auto& context = resolveFlagGroup(flagPath);
    const auto opt = context.getOptionDynamic<MultiOption<Container>>(flagPath.flag);
    if (opt == nullptr) {
        throw InvalidFlagPathException(flagPath);
    }
    return opt->getValue();
}

inline auto Context::setName(const std::string& name) -> void {
    m_name = name;
}

inline auto Context::getPath() const -> std::string {
    if (m_parent == nullptr) {
        return "";
    }

    std::vector<const std::string*> names;

    const auto *current = this;
    while (current != nullptr) {
        if (!current->m_name.empty()) {
            names.push_back(&current->m_name);
        }
        current = current->m_parent;
    }

    std::ranges::reverse(names);
    return std::accumulate(std::next(names.begin()), names.end(), *(names[0]),
        [] (const std::string& name1, const std::string *name2) {
            return name1 + " > " + *name2;
    });
}

inline auto Context::containsLocalFlag(const std::string& flag) const -> bool {
    return std::ranges::contains(m_flags, flag);
}

inline auto Context::resolveFlagGroup(const FlagPath& flagPath) -> Context& {
    auto context = this;
    // Loop through all the flags that represent groups
    for (const auto& groupFlag : flagPath.groupPath) {
        const auto optGroup = context->getOptionDynamic<OptionGroup>(groupFlag);
        if (optGroup == nullptr) {
            throw InvalidFlagPathException(flagPath);
        }
        context = &optGroup->getContext();
    }
    return *context;
}
} // End namespace Argon

#endif // ARGON_CONTEXT_INCLUDE