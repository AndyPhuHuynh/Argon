#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Error.hpp"
#include "Flag.hpp"
#include "ParserConfig.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename OptionType> class OptionHolder;
    class IsPositional;

    using OptionMap = std::unordered_map<FlagPathWithAlias, const IOption*>;

    class Context {
        std::vector<OptionHolder<IOption>> m_options;
        std::vector<OptionHolder<IOption>> m_positionals;

        PositionalPolicy m_positionalPolicy = PositionalPolicy::UseDefault;
    public:
        Context() = default;

        Context(const Context&) = default;
        auto operator=(const Context&) -> Context& = default;

        Context(Context&&) noexcept = default;
        auto operator=(Context&&) noexcept -> Context& = default;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto getFlagOption(std::string_view flag) -> IOption *;

        auto getFlagOption(const FlagPath& flagPath) -> IOption *;

        [[nodiscard]] auto getPositional(size_t position) -> IsPositional *;

        template <typename T>
        auto getOptionDynamic(std::string_view flag) -> T *;

        [[nodiscard]] auto containsLocalFlag(std::string_view flag) const -> bool;

        [[nodiscard]] auto getHelpMessage(size_t maxLineWidth = 120) const -> std::string;

        template<typename ValueType>
        auto getOptionValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue() -> const ValueType&;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue(const FlagPath& groupPath) -> const ValueType&;

        [[nodiscard]] auto collectAllSetOptions() const -> OptionMap;

        auto validate(ErrorGroup& errorGroup, std::string_view shortPrefix, std::string_view longPrefix) const -> void;

        [[nodiscard]] auto getPositionalPolicy() const -> PositionalPolicy;

        auto setPositionalPolicy(PositionalPolicy newPolicy) -> void;
    private:
        template <typename T> requires DerivesFrom<T, IsPositional>
        auto addPositionalOption(T&& option) -> void;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addNonPositionalOption(T&& option) -> void;

        [[nodiscard]] auto collectAllFlags() const -> std::vector<const Flag*>;

        auto resolveFlagGroup(const FlagPath& flagPath) -> Context&;

        auto getHelpMessage(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth) const -> void;

        static auto getHelpMessage(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth, const IOption *option) -> void;

        auto collectAllSetOptions(OptionMap& map, const std::vector<Flag>& pathSoFar) const -> void;

        auto validate(const FlagPath& pathSoFar, ErrorGroup& validationErrors, std::string_view shortPrefix, std::string_view longPrefix) const -> void;

        static auto checkPrefixes(const std::vector<const Flag *>& flags, ErrorGroup& validationErrors, std::string_view shortPrefix, std::string_view longPrefix) -> void;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {

inline auto containsFlagPath(const OptionMap& map, const FlagPath& flag) -> const FlagPathWithAlias * {
    for (const auto& flagWithAlias : map | std::views::keys) {
        if (flagWithAlias == flag) return &flagWithAlias;
    }
    return nullptr;
}

inline auto startsWithAny(const std::string_view str, const std::initializer_list<std::string_view> prefixes) -> bool {
    return std::ranges::any_of(prefixes, [str](const std::string_view prefix) {
        return str.starts_with(prefix);
    });
}

} // End namespace Argon

//---------------------------------------------------Implementations----------------------------------------------------

#include <algorithm>
#include <iomanip>
#include <set>

#include "Argon/Options/Option.hpp"
#include "Argon/Options/OptionGroup.hpp"
#include "Argon/Options/OptionHolder.hpp"
#include "Argon/Options/MultiOption.hpp"
#include "Argon/Options/Positional.hpp"

namespace Argon {

template<typename T> requires DerivesFrom<T, IOption>
auto Context::addOption(T&& option) -> void {
    if constexpr (DerivesFrom<T, IsPositional>) {
        addPositionalOption(std::forward<T>(option));
    } else {
        addNonPositionalOption(std::forward<T>(option));
    }
}

template<typename T> requires DerivesFrom<T, IsPositional>
auto Context::addPositionalOption(T&&option) -> void {
    m_positionals.emplace_back(std::forward<T>(option));
}

template<typename T> requires DerivesFrom<T, IOption>
auto Context::addNonPositionalOption(T&& option) -> void {
    m_options.emplace_back(std::forward<T>(option));
}

inline auto Context::getFlagOption(const std::string_view flag) -> IOption * {
    const auto it = std::ranges::find_if(m_options, [&flag](const OptionHolder<IOption>& holder) {
        const auto optPtr = holder.getPtr();
        if (!dynamic_cast<const IFlag*>(optPtr)) {
            throw std::invalid_argument("Unsupported option type");
        }
        return dynamic_cast<const IFlag*>(optPtr)->getFlag().containsFlag(flag);
    });

    return it == m_options.end() ? nullptr : it->getPtr();
}

inline auto Context::getFlagOption(const FlagPath& flagPath) -> IOption * {
    auto& context = resolveFlagGroup(flagPath);
    return context.getFlagOption(flagPath.flag);
}

inline auto Context::getPositional(const size_t position) -> IsPositional * {
    if (position >= m_positionals.size()) return nullptr;
    const auto ptr = dynamic_cast<IsPositional *>(m_positionals[position].getPtr());
    assert(ptr != nullptr && "Non positional option found in the positionals list");
    return ptr;
}

template <typename T>
auto Context::getOptionDynamic(const std::string_view flag) -> T * {
    return dynamic_cast<T*>(getFlagOption(flag));
}

template<typename ValueType>
auto Context::getOptionValue(const FlagPath& flagPath) -> const ValueType& {
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

template<typename ValueType, size_t Pos>
auto Context::getPositionalValue() -> const ValueType& {
    if (Pos >= m_positionals.size()) {
        throw std::invalid_argument(std::format(
            "Max positional is {}, attempted to get position {}",
            m_positionals.size() - 1, Pos));
    }
    const auto ptr = dynamic_cast<Positional<ValueType> *>(m_positionals[Pos].getPtr());
    if (!ptr) {
        throw std::invalid_argument(std::format("Positional at position {} is not of the specified type", Pos));
    }
    return ptr->getValue();
}

template<typename ValueType, size_t Pos>
auto Context::getPositionalValue(const FlagPath& groupPath) -> const ValueType& {
    const auto iOption = getFlagOption(groupPath);
    if (iOption == nullptr) {
        throw InvalidFlagPathException(groupPath);
    }
    const auto groupPtr = dynamic_cast<OptionGroup*>(iOption);
    if (groupPtr == nullptr) {
        throw std::invalid_argument(std::format("Given flag path is not an Option Group: {}", groupPath.getString()));
    }
    return groupPtr->getContext().getPositionalValue<ValueType, Pos>();
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

    for (const auto& holder : m_options) {
        const auto optPtr = holder.getPtr();

        if (!dynamic_cast<const IFlag*>(optPtr)) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(optPtr); groupPtr != nullptr) {
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

inline auto Context::collectAllSetOptions() const -> OptionMap {
    OptionMap result;
    collectAllSetOptions(result, std::vector<Flag>());
    return result;
}

inline auto Context::collectAllSetOptions(OptionMap& map, //NOLINT (recursion)
                                          const std::vector<Flag>& pathSoFar) const -> void {
    for (const auto& holder : m_options) {
        const IOption *optPtr = holder.getPtr();

        auto *flagPtr = dynamic_cast<const IFlag*>(optPtr);
        if (!flagPtr) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(optPtr); groupPtr != nullptr) {
            auto newVec = pathSoFar;
            newVec.emplace_back(flagPtr->getFlag());
            groupPtr->getContext().collectAllSetOptions(map, newVec);
            continue;
        }

        if (!optPtr->isSet()) continue;

        map[FlagPathWithAlias(pathSoFar, flagPtr->getFlag())] = optPtr;
    }
}

inline auto Context::validate(
    ErrorGroup& errorGroup,
    const std::string_view shortPrefix, const std::string_view longPrefix
) const -> void {
    validate(FlagPath(), errorGroup, shortPrefix, longPrefix);
}

inline auto Context::validate( // NOLINT (misc-no-recursion)
    const FlagPath& pathSoFar, ErrorGroup& validationErrors,
    const std::string_view shortPrefix, const std::string_view longPrefix
) const -> void {
    const auto allFlags = collectAllFlags();
    checkPrefixes(allFlags, validationErrors, shortPrefix, longPrefix);
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
            validationErrors.addErrorMessage(
                std::format("Multiple flags found with the value of '{}'", flag),
                -1, ErrorType::Validation_DuplicateFlag);
        } else {
            validationErrors.addErrorMessage(std::format(
                "Multiple flags found with the value of '{}' within group '{}'", flag, pathSoFar.getString()),
                -1, ErrorType::Validation_DuplicateFlag);
        }
    }

    for (const auto& holder : m_options) {
        const IOption *ptr = holder.getPtr();

        if (!dynamic_cast<const IFlag*>(ptr)) {
            throw std::invalid_argument("Unsupported option type");
        }

        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(ptr); groupPtr != nullptr) {
            FlagPath newPath = pathSoFar;
            newPath.extendPath(groupPtr->getFlag().mainFlag);
            groupPtr->getContext().validate(newPath, validationErrors, shortPrefix, longPrefix);
        }
    }
}

inline auto Context::checkPrefixes(
    const std::vector<const Flag *>& flags, ErrorGroup& validationErrors,
    const std::string_view shortPrefix, const std::string_view longPrefix
) -> void {
    for (const auto& flag : flags) {
        if (!startsWithAny(flag->mainFlag, {shortPrefix, longPrefix})) {
            validationErrors.addErrorMessage(
                std::format("Flag '{}' does not start with a flag prefix", flag->mainFlag),
                -1, ErrorType::Validation_NoPrefix);
        }
        for (const auto& alias : flag->aliases) {
            if (!startsWithAny(alias, {shortPrefix, longPrefix})) {
                validationErrors.addErrorMessage(
                std::format("Flag '{}' does not start with a flag prefix", alias),
                -1, ErrorType::Validation_NoPrefix);
            }
        }
    }
}

inline auto Context::getPositionalPolicy() const -> PositionalPolicy {
    return m_positionalPolicy;
}

inline auto Context::setPositionalPolicy(const PositionalPolicy newPolicy) -> void {
    m_positionalPolicy = newPolicy;
}

inline auto Context::collectAllFlags() const -> std::vector<const Flag*> {
    std::vector<const Flag*> result;
    for (const auto& holder : m_options) {
        const IOption *ptr = holder.getPtr();
        if (!dynamic_cast<const IFlag*>(ptr)) {
            throw std::invalid_argument("Argon internal error: option without flag in m_options");
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