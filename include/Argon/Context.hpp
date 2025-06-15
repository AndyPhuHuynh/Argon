#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
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
        std::string m_name;
        Context *m_parent = nullptr;
    public:
        Context() = default;
        explicit Context(std::string name);
        Context(const Context&);
        auto operator=(const Context&) -> Context&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto getOption(std::string_view flag) -> IOption*;

        auto getOption(const FlagPath& flagPath) -> IOption*;

        template <typename T>
        auto getOptionDynamic(const std::string& flag) -> T *;

        auto setName(const std::string& name) -> void;

        [[nodiscard]] auto getPath() const -> std::string;

        [[nodiscard]] auto containsLocalFlag(std::string_view flag) const -> bool;

        [[nodiscard]] auto getHelpMessage(size_t maxLineWidth = 120) const -> std::string;

        template<typename ValueType>
        auto getValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        auto collectAllSetOptions() -> OptionMap;

        auto validate(std::vector<std::string>& errorMsgs) -> void;

        auto applyPrefixes(std::string_view shortPrefix, std::string_view longPrefix) -> void;

    private:
        [[nodiscard]] auto collectAllFlags() const -> std::vector<const Flag*>;

        auto resolveFlagGroup(const FlagPath& flagPath) -> Context&;

        auto getHelpMessage(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth) const -> void;

        static auto getHelpMessage(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth, const IOption *option) -> void;

        auto collectAllSetOptions(OptionMap& map, const std::vector<Flag>& pathSoFar) -> void;

        auto validate(const FlagPath& pathSoFar, std::vector<std::string>& errorMsgs) -> void;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
inline auto getRawPointer(const OptionPtr& optPtr) -> IOption * {
    return std::visit([]<typename T>(const T& opt) -> IOption * {
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
#include <iomanip>
#include <set>

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
    m_name = other.m_name;
    m_parent = other.m_parent;
    return *this;
}

template<typename T> requires DerivesFrom<T, IOption>
auto Context::addOption(T&& option) -> void {
    IOption *optPtr;
    // Non-owned option
    if (std::is_lvalue_reference_v<T>) {
        m_options.emplace_back(&option);
        optPtr = &option;
    }
    // Owned option
    else {
        optPtr = getRawPointer(m_options.emplace_back(option.clone()));
    }

    if (const auto optionGroup = dynamic_cast<OptionGroup*>(optPtr); optionGroup != nullptr) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Context::getOption(const std::string_view flag) -> IOption * {
    const auto it = std::ranges::find_if(m_options, [&flag](const OptionPtr& option) {
        const auto optPtr = getRawPointer(option);
        if (!dynamic_cast<const IFlag*>(optPtr)) {
            throw std::invalid_argument("Unsupported option type");
        }
        return dynamic_cast<const IFlag*>(optPtr)->getFlag().containsFlag(flag);
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
    const auto allFlags = collectAllFlags();
    return std::ranges::any_of(allFlags, [&flag](const Flag* flagInList) {
        return flagInList->containsFlag(flag);
    });
}

inline auto Context::getHelpMessage(const size_t maxLineWidth) const -> std::string {
    std::stringstream ss;
    ss << "Usage: [options]\n\n" << "Options:\n" << std::string(maxLineWidth, '-') << "\n";
    getHelpMessage(ss, 0, maxLineWidth);
    return ss.str();
}

inline auto Context::getHelpMessage(std::stringstream& ss, const size_t leadingSpaces, const size_t maxLineWidth) const -> void { //NOLINT (recursion)
    std::vector<const OptionGroup*> groups;

    for (const auto& opt : m_options) {
        const auto optPtr = getRawPointer(opt);

        if (!dynamic_cast<const IFlag*>(optPtr)) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<OptionGroup*>(optPtr); groupPtr != nullptr) {
            groups.push_back(groupPtr);
            continue;
        }
        getHelpMessage(ss, leadingSpaces, maxLineWidth, optPtr);
    }

    // Print top level group help messages
    for (const auto& group: groups) {
        getHelpMessage(ss, leadingSpaces, maxLineWidth, group);
    }

    // Print nested group messages
    const auto leading = std::string(leadingSpaces + 4, ' ');
    for (const auto& group : groups) {
        ss << "\n" << leading;
        if (const auto& inputHint = group->getInputHint(); !inputHint.empty()) {
            ss << inputHint;
        } else {
            ss << std::format("[{}]", group->getFlag().getString());
        }
        ss << "\n" << leading << std::string(maxLineWidth - leadingSpaces - 4, '-') << "\n";
        group->getContext().getHelpMessage(ss, leadingSpaces + 4, maxLineWidth);
    }
}

inline auto Context::getHelpMessage(std::stringstream& ss, const size_t leadingSpaces,
                                    const size_t maxLineWidth, const IOption *option) -> void {
    if (!dynamic_cast<const IFlag*>(option)) {
        throw std::invalid_argument("Unsupported option type");
    }
    // Print flag
    ss << std::string(leadingSpaces, ' ');
    constexpr size_t maxFlagWidth = 32;
    std::string flag = dynamic_cast<const IFlag*>(option)->getFlag().getString();
    if (const auto& typeHint = option->getInputHint(); !typeHint.empty()) {
        flag += ' ';
        flag += typeHint;
    }
    flag += ':';
    ss << std::left << std::setw(maxFlagWidth) << flag;

    // Print description with line wrapping
    const std::string& description = option->getDescription();
    std::vector<std::string_view> sections;

    const size_t maxDescLength = maxLineWidth - maxFlagWidth;
    size_t sectionStart = 0;
    while (sectionStart < description.length()) {
        // Skip any leading whitespace
        while (sectionStart < description.length() && description[sectionStart] == ' ') {
            ++sectionStart;
        }

        if (sectionStart >= description.length()) {
            break;
        }

        // Determine the maximum end index
        const size_t end = sectionStart + maxDescLength;
        if (end >= description.length()) {
            // Add the final section
            sections.emplace_back(&description[sectionStart], description.length() - sectionStart);
            break;
        }

        // Find the last space within the range
        size_t breakPoint = description.rfind(' ', end);
        if (breakPoint == std::string::npos || breakPoint <= sectionStart) {
            // No space found, or the space is before sectionStart – hard break
            breakPoint = end;
        }

        size_t length = breakPoint - sectionStart;
        sections.emplace_back(&description[sectionStart], length);
        sectionStart = breakPoint;
    }

    if (flag.length() > maxFlagWidth) {
        ss << "\n" << std::string(leadingSpaces + maxFlagWidth, ' ') << sections[0];
    } else {
        ss << sections[0];
    }
    ss << "\n";

    for (size_t i = 1; i < sections.size(); i++) {
        if (sections[i].empty()) continue;
        ss << std::string(leadingSpaces + maxFlagWidth, ' ') << sections[i] << "\n";
    }
}

inline auto Context::collectAllSetOptions() -> OptionMap {
    std::unordered_map<FlagPathWithAlias, IOption *> result;
    collectAllSetOptions(result, std::vector<Flag>());
    return result;
}

inline auto Context::collectAllSetOptions(OptionMap& map, //NOLINT (recursion)
                                          const std::vector<Flag>& pathSoFar) -> void {
    for (const auto& opt : m_options) {
        IOption *optPtr = getRawPointer(opt);

        auto *flagPtr = dynamic_cast<IFlag*>(optPtr);
        if (!flagPtr) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<OptionGroup*>(optPtr); groupPtr != nullptr) {
            auto newVec = pathSoFar;
            newVec.emplace_back(flagPtr->getFlag());
            groupPtr->getContext().collectAllSetOptions(map, newVec);
            continue;
        }

        if (!optPtr->isSet()) continue;

        map[FlagPathWithAlias(pathSoFar, flagPtr->getFlag())] = optPtr;
    }
}

inline auto Context::validate(std::vector<std::string>& errorMsgs) -> void {
    validate(FlagPath(), errorMsgs);
}

inline auto Context::validate(const FlagPath& pathSoFar, std::vector<std::string>& errorMsgs) -> void { //NOLINT (recursion)
    const auto allFlags = collectAllFlags();
    std::set<std::string> flags;
    std::set<std::string> duplicateFlags;
    for (const auto& flag : allFlags) {
        if (flags.contains(flag->mainFlag)) {
            duplicateFlags.emplace(flag->mainFlag);
        } else {
            flags.emplace(flag->mainFlag);
        }

        for (const auto& alias : flag->aliases) {
            if (flags.contains(alias)) {
                duplicateFlags.emplace(alias);
            } else {
                flags.emplace(alias);
            }
        }
    }

    for (const auto& flag : duplicateFlags) {
        if (pathSoFar.flag.empty()) {
            errorMsgs.emplace_back(std::format("Multiple flags found with the value of '{}'", flag));
        } else {
            errorMsgs.emplace_back(std::format(
                "Multiple flags found with the value of '{}' within group '{}'",
                flag, pathSoFar.getString()));
        }
    }

    for (const auto& opt : m_options) {
        IOption *ptr = getRawPointer(opt);

        if (!dynamic_cast<const IFlag*>(ptr)) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<OptionGroup*>(ptr); groupPtr != nullptr) {
            FlagPath newPath = pathSoFar;
            newPath.extendPath(groupPtr->getFlag().mainFlag);
            groupPtr->getContext().validate(newPath, errorMsgs);
        }
    }
}

inline auto Context::applyPrefixes(const std::string_view shortPrefix,  // NOLINT (const)
                                   const std::string_view longPrefix) -> void {
    for (const auto& opt : m_options) {
        IOption *ptr = getRawPointer(opt);
        if (!dynamic_cast<const IFlag*>(ptr)) {
            throw std::invalid_argument("Unsupported option type");
        }
        dynamic_cast<IFlag*>(ptr)->applyPrefixes(shortPrefix, longPrefix);
        if (const auto groupPtr = dynamic_cast<OptionGroup*>(ptr); groupPtr != nullptr) {
            groupPtr->getContext().applyPrefixes(shortPrefix, longPrefix);
        }
    }
}

inline auto Context::collectAllFlags() const -> std::vector<const Flag*> {
    std::vector<const Flag*> result;
    for (const auto& opt : m_options) {
        const IOption *ptr = getRawPointer(opt);
        if (!dynamic_cast<const IFlag*>(ptr)) {
            throw std::invalid_argument("Unsupported option type");
        }
        result.emplace_back(&dynamic_cast<const IFlag*>(ptr)->getFlag());
    }
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
} // End namespace Argon

#endif // ARGON_CONTEXT_INCLUDE