#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Flag.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    using OptionPtr = std::variant<IOption*, std::unique_ptr<IOption>>;
    using OptionMap = std::unordered_map<FlagPathWithAlias, IOption*>;

    class Context {
        std::vector<OptionPtr> m_options;
        std::vector<Flag> m_flags;
        std::string m_name;
        Context *m_parent = nullptr;
    public:
        Context() = default;
        explicit Context(std::string name);
        Context(const Context&);
        auto operator=(const Context&) -> Context&;
        
        auto addOption(IOption& option) -> void;

        auto addOption(IOption&& option) -> void;

        auto getOption(std::string_view flag) -> IOption*;

        auto getOption(const FlagPath& flagPath) -> IOption*;

        template <typename T>
        auto getOptionDynamic(const std::string& flag) -> T*;

        auto setName(const std::string& name) -> void;

        [[nodiscard]] auto getPath() const -> std::string;

        [[nodiscard]] auto containsLocalFlag(std::string_view flag) const -> bool;

        template<typename ValueType>
        auto getValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        auto collectAllSetOptions() -> OptionMap;

    private:
        auto resolveFlagGroup(const FlagPath& flagPath) -> Context&;

        auto collectAllSetOptions(OptionMap& map, const std::vector<Flag>& pathSoFar) -> void;
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

inline auto containsFlag(const OptionMap& map, const FlagPath& flag) -> const FlagPathWithAlias * {
    for (const auto& flagWithAlias : map | std::views::keys) {
        if (flagWithAlias == flag) return &flagWithAlias;
    }
    return nullptr;
}
} // End namespace Argon

//---------------------------------------------------Implementations----------------------------------------------------

#include <algorithm>

#include "Option.hpp" // NOLINT (misc-unused-include)
#include "MultiOption.hpp"

namespace Argon {

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
    m_flags.emplace_back(option.getFlag());
    m_options.emplace_back(&option);
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(&option)) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Context::addOption(IOption&& option) -> void {
    m_flags.emplace_back(option.getFlag());
    const auto& newOpt = m_options.emplace_back(option.clone());
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(newOpt))) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Context::getOption(const std::string_view flag) -> IOption * {
    const auto it = std::ranges::find_if(m_options, [&flag](const OptionPtr& option) {
        return std::visit([&flag]<typename T>(const T& opt) -> bool {
            return opt->getFlag().containsFlag(flag);
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

inline auto Context::containsLocalFlag(const std::string_view flag) const -> bool {
    return std::ranges::any_of(m_flags, [&flag](const Flag& flagInList) {
        return flagInList.containsFlag(flag);
    });
}

inline auto Context::collectAllSetOptions() -> OptionMap {
    std::unordered_map<FlagPathWithAlias, IOption *> result;
    collectAllSetOptions(result, std::vector<Flag>());
    return result;
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

inline auto Context::collectAllSetOptions(OptionMap& map,
                                          const std::vector<Flag>& pathSoFar) -> void {
    for (const auto& opt : m_options) {
        std::visit([&]<typename T>(const T& optPtr) {
            IOption *ptr = nullptr;
            if constexpr (std::is_same_v<T, IOption*>) {
                ptr = optPtr;
            } else if constexpr (std::is_same_v<T, std::unique_ptr<IOption>>) {
                ptr = optPtr.get();
            }
            if (ptr == nullptr) {
                throw std::runtime_error("Unhandled type in collectAllSetOptions()");
            }
            if (const auto groupPtr = dynamic_cast<OptionGroup*>(ptr); groupPtr != nullptr) {
                auto newVec = pathSoFar;
                newVec.emplace_back(optPtr->getFlag());
                groupPtr->getContext().collectAllSetOptions(map, newVec);
                return;
            }

            if (!ptr->isSet()) return;

            map[FlagPathWithAlias(pathSoFar, ptr->getFlag())] = ptr;

        }, opt);
    }
}

} // End namespace Argon

#endif // ARGON_CONTEXT_INCLUDE