#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Error.hpp"
#include "Flag.hpp"
#include "Config/ContextConfig.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename OptionType> class OptionHolder;
    class IsPositional;

    using OptionMap = std::unordered_map<FlagPathWithAlias, const IOption*>;

    class Context final : public detail::ContextConfigForwarder<Context>{
        std::vector<OptionHolder<IOption>> m_options;
        std::vector<OptionHolder<IOption>> m_positionals;
    public:
        ContextConfig config = ContextConfig(true);

        // Context() = default;
        explicit Context(const bool allowConfigUseDefault) : config(allowConfigUseDefault) {}

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

        [[nodiscard]] auto getHelpMessage(size_t maxLineWidth, PositionalPolicy defaultPolicy) const -> std::string;

        template<typename ValueType>
        auto getOptionValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue() -> const ValueType&;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue(const FlagPath& groupPath) -> const ValueType&;

        [[nodiscard]] auto collectAllSetOptions() const -> OptionMap;

        [[nodiscard]] auto getOptionsVector() const -> const std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getPositionalsVector() const -> const std::vector<OptionHolder<IOption>>&;

        auto validate(ErrorGroup& errorGroup) const -> void;

    private:
        [[nodiscard]] auto getConfigImpl() -> ContextConfig& override;

        [[nodiscard]] auto getConfigImpl() const -> const ContextConfig& override;

        template <typename T> requires DerivesFrom<T, IsPositional>
        auto addPositionalOption(T&& option) -> void;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addNonPositionalOption(T&& option) -> void;

        [[nodiscard]] auto collectAllFlags() const -> std::vector<const Flag*>;

        auto resolveFlagGroup(const FlagPath& flagPath) -> Context&;

        auto getHelpMessage(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth, PositionalPolicy defaultPolicy) const -> void;

        static auto getHelpMessageForOption(std::stringstream& ss, size_t leadingSpaces, size_t maxLineWidth, const IOption *option, PositionalPolicy defaultPolicy) -> void;

        auto collectAllSetOptions(OptionMap& map, const std::vector<Flag>& pathSoFar) const -> void;

        auto validate(const FlagPath& pathSoFar, ErrorGroup& validationErrors) const -> void;

        auto checkPrefixes(ErrorGroup& validationErrors) const -> void;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

#include <algorithm>
#include <iomanip>
#include <set>

#include "Argon/Options/Option.hpp"
#include "Argon/Options/OptionGroup.hpp"
#include "Argon/Options/OptionHolder.hpp"
#include "Argon/Options/MultiOption.hpp"
#include "Argon/Options/Positional.hpp"

namespace Argon::detail {

inline auto containsFlagPath(const OptionMap& map, const FlagPath& flag) -> const FlagPathWithAlias * {
    for (const auto& flagWithAlias : map | std::views::keys) {
        if (flagWithAlias == flag) return &flagWithAlias;
    }
    return nullptr;
}

inline auto startsWithAny(const std::string_view str, const std::vector<std::string>& prefixes) -> bool {
    return std::ranges::any_of(prefixes, [str](const std::string_view prefix) {
        return str.starts_with(prefix);
    });
}

auto assertHasFlag(const auto *opt) -> void {
    assert(dynamic_cast<const IFlag*>(opt) != nullptr && "Argon internal error: option does not have a flag");
}

inline auto getIFlag(OptionHolder<IOption>& holder) -> IFlag * {
    const auto iFlag = dynamic_cast<IFlag *>(holder.getPtr());
    assert(iFlag != nullptr && "Argon internal error: option without flag in m_options");
    return iFlag;
}

inline auto getIFlag(const OptionHolder<IOption>& holder) -> const IFlag * {
    const auto iFlag = dynamic_cast<const IFlag *>(holder.getPtr());
    assert(iFlag != nullptr && "Argon internal error: option without flag in m_options");
    return iFlag;
}

inline auto getOptionName(const IOption *option) -> std::string {
    if (const auto flag = dynamic_cast<const IFlag *>(option)) {
        return flag->getFlag().getString();
    }
    return option->getInputHint();
}

inline auto getFullInputHint(const IOption *option, const PositionalPolicy defaultPositionalPolicy) -> std::string {
    auto concatPositionals = [](std::stringstream& ss, const std::vector<OptionHolder<IOption>>& positionals) {
        if (positionals.empty()) return;
        ss << ' ';
        for (size_t i = 0; i < positionals.size(); ++i) {
            ss << positionals[i].getRef().getInputHint();
            if (i < positionals.size() - 1) {
                ss << ' ';
            }
        }
    };

    auto concatTypeHint = [](std::stringstream& ss, const IOption *opt) {
        if (const auto& typeHint = opt->getInputHint(); !typeHint.empty()) {
            ss << ' ' << typeHint;
        } else if (const auto group = dynamic_cast<const OptionGroup *>(opt); group != nullptr) {
            ss << " [" << group->getFlag().mainFlag << "]";
        }
    };

    std::stringstream name;

    if (const auto group = dynamic_cast<const OptionGroup *>(option); group != nullptr) {
        const auto positionals = group->getContext().getPositionalsVector();
        const auto policy = resolvePositionalPolicy(defaultPositionalPolicy, group->getContext().getDefaultPositionalPolicy());
        switch (policy) {
            case PositionalPolicy::BeforeFlags:
                concatPositionals(name, positionals);
                concatTypeHint(name, option);
                break;
            case PositionalPolicy::Interleaved:
            case PositionalPolicy::AfterFlags:
                concatTypeHint(name, option);
                concatPositionals(name, positionals);
                break;
            case PositionalPolicy::UseDefault:
                std::unreachable();
        };
    } else if (dynamic_cast<const IFlag *>(option)) {
        concatTypeHint(name, option);
    }
    return name.str();
}

} // End namespace Argon::detail

//---------------------------------------------------Implementations----------------------------------------------------

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
        const auto iFlag = detail::getIFlag(holder);
        return iFlag->getFlag().containsFlag(flag);
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
        throw InvalidFlagPathException(std::format("Given flag path is not an Option Group: '{}'", groupPath.getString()));
    }
    return groupPtr->getContext().getPositionalValue<ValueType, Pos>();
}

inline auto Context::containsLocalFlag(const std::string_view flag) const -> bool {
    const auto allFlags = collectAllFlags();
    return std::ranges::any_of(allFlags, [&flag](const Flag* flagInList) {
        return flagInList->containsFlag(flag);
    });
}

inline auto Context::getHelpMessage(const size_t maxLineWidth, const PositionalPolicy defaultPolicy) const -> std::string {
    std::stringstream ss;
    ss << "Usage: [options]\n\n" << "Options:\n" << std::string(maxLineWidth, '-') << "\n";
    getHelpMessage(ss, 0, maxLineWidth, defaultPolicy);
    return ss.str();
}

inline auto Context::getHelpMessage( // NOLINT (misc-no-recursion)
    std::stringstream& ss, const size_t leadingSpaces, const size_t maxLineWidth, const PositionalPolicy defaultPolicy
) const -> void {
    std::vector<const OptionGroup*> groups;
    auto leading = std::string(leadingSpaces, ' ');
    bool sectionBeforeSet = false;

    if (!m_options.empty()) {
        ss << leading << "Options:\n";
        sectionBeforeSet = true;
    }
    for (const auto& holder : m_options) {
        const auto optPtr = holder.getPtr();
        detail::assertHasFlag(optPtr);
        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(optPtr); groupPtr != nullptr) {
            groups.push_back(groupPtr);
            continue;
        }
        getHelpMessageForOption(ss, leadingSpaces, maxLineWidth, optPtr, defaultPolicy);
    }

    if (!m_positionals.empty()) {
        if (sectionBeforeSet) ss << "\n";
        ss << leading << "Positionals:\n";
        sectionBeforeSet = true;
    }
    for (const auto& holder : m_positionals) {
        getHelpMessageForOption(ss, leadingSpaces, maxLineWidth, holder.getPtr(), defaultPolicy);
    }

    // Print group messages
    if (!groups.empty()) {
        if (sectionBeforeSet) ss << "\n";
        ss << leading << "Groups:\n";
        leading += "    ";
    }
    for (const auto& group : groups) {
        getHelpMessageForOption(ss, leadingSpaces, maxLineWidth, group, defaultPolicy);
        ss << "\n" << leading;

        if (const auto& inputHint = group->getInputHint(); !inputHint.empty()) {
            ss << inputHint;
        } else {
            ss << std::format("[{}]", group->getFlag().getString());
        }

        ss << "\n" << leading << std::string(maxLineWidth - leadingSpaces - 4, '-') << "\n";
        group->getContext().getHelpMessage(ss, leadingSpaces + 4, maxLineWidth, defaultPolicy);
    }
}

inline auto Context::getHelpMessageForOption(
    std::stringstream& ss, size_t leadingSpaces, const size_t maxLineWidth, const IOption *option,
    const PositionalPolicy defaultPolicy
) -> void {
    leadingSpaces += 2;
    constexpr size_t maxFlagWidthBeforeWrapping = 32;

    // Print name
    const auto namePadding = std::string(leadingSpaces, ' ');
    const auto name = detail::getOptionName(option);
    const auto inputPadding = std::string(leadingSpaces + name.length(), ' ');
    const auto inputHint = detail::getFullInputHint(option, defaultPolicy);
    const auto inputSections = detail::wrapString(inputHint, maxLineWidth - inputPadding.length());
    ss << namePadding << name;
    if (inputSections.empty()) {
        const auto width = static_cast<std::streamsize>(maxFlagWidthBeforeWrapping - name.length());
        ss << std::left << std::setw(width) << ':';
    } else {
        for (size_t i = 0; i < inputSections.size(); i++) {
            const auto width = static_cast<std::streamsize>(maxFlagWidthBeforeWrapping - name.length());
            std::string buffer = " " + inputSections[i];
            buffer += i == inputSections.size() - 1 ? ':' : '\n';
            if (i != 0) {
                ss << inputPadding;
            }
            ss << std::left << std::setw(width) << buffer;
        }
    }

    // Print description with line wrapping
    const std::string& description = option->getDescription();

    const size_t maxDescLength = maxLineWidth - maxFlagWidthBeforeWrapping;
    const std::vector<std::string> sections = detail::wrapString(description, maxDescLength);

    // +1 in flag length is for the semicolon
    if (const auto flagLength = name.length() + inputHint.length() + 1; flagLength > maxFlagWidthBeforeWrapping) {
        ss << "\n" << std::string(leadingSpaces + maxFlagWidthBeforeWrapping, ' ') << sections[0];
    } else {
        ss << sections[0];
    }
    ss << "\n";

    for (size_t i = 1; i < sections.size(); i++) {
        if (sections[i].empty()) continue;
        ss << std::string(leadingSpaces + maxFlagWidthBeforeWrapping, ' ') << sections[i] << "\n";
    }
}

inline auto Context::collectAllSetOptions() const -> OptionMap {
    OptionMap result;
    collectAllSetOptions(result, std::vector<Flag>());
    return result;
}

inline auto Context::getOptionsVector() const -> const std::vector<OptionHolder<IOption>>& {
    return m_options;
}

inline auto Context::getPositionalsVector() const -> const std::vector<OptionHolder<IOption>>& {
    return m_positionals;
}

inline auto Context::collectAllSetOptions(OptionMap& map, //NOLINT (recursion)
                                          const std::vector<Flag>& pathSoFar) const -> void {
    for (const auto& holder : m_options) {
        const IOption *optPtr = holder.getPtr();
        auto *flagPtr = detail::getIFlag(holder);
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

inline auto Context::validate(ErrorGroup& errorGroup) const -> void {
    validate(FlagPath(), errorGroup);
}

inline auto Context::getConfigImpl() -> ContextConfig& {
    return config;
}

inline auto Context::getConfigImpl() const -> const ContextConfig& {
    return config;
}

inline auto Context::validate( // NOLINT (misc-no-recursion)
    const FlagPath& pathSoFar, ErrorGroup& validationErrors
) const -> void {
    const auto allFlags = collectAllFlags();
    checkPrefixes(validationErrors);
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
        const IOption& ref = holder.getRef();
        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(&ref); groupPtr != nullptr) {
            FlagPath newPath = pathSoFar;
            newPath.extendPath(groupPtr->getFlag().mainFlag);
            groupPtr->getContext().validate(newPath, validationErrors);
        }
    }
}

inline auto Context::checkPrefixes(ErrorGroup& validationErrors) const -> void {
    for (const auto& holder : m_options) {
        const auto flag = detail::getIFlag(holder);
        if (!detail::startsWithAny(flag->getFlag().mainFlag, config.getFlagPrefixes())) {
            validationErrors.addErrorMessage(
                std::format("Flag '{}' does not start with a flag prefix", flag->getFlag().mainFlag),
                -1, ErrorType::Validation_NoPrefix);
        }
        for (const auto& alias : flag->getFlag().aliases) {
            if (!detail::startsWithAny(alias, config.getFlagPrefixes())) {
                validationErrors.addErrorMessage(
                std::format("Flag '{}' does not start with a flag prefix", alias),
                -1, ErrorType::Validation_NoPrefix);
            }
        }
    }
}

inline auto Context::collectAllFlags() const -> std::vector<const Flag*> {
    std::vector<const Flag*> result;
    for (const auto& holder : m_options) {
        const auto iFlag = detail::getIFlag(holder);
        result.emplace_back(&iFlag->getFlag());
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