#ifndef ARGON_CONTEXT_INCLUDE
#define ARGON_CONTEXT_INCLUDE

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "Argon/Config/ContextConfig.hpp"
#include "Argon/Config/ContextConfigForwarder.hpp"
#include "Argon/Error.hpp"
#include "Argon/Flag.hpp"
#include "Argon/Traits.hpp"

namespace Argon {
    class IOption;
    class OptionGroup;
    template <typename OptionType> class OptionHolder;
    class IsPositional;

    using OptionMap = std::unordered_map<FlagPathWithAlias, const IOption*>;

    class Context final : public detail::ContextConfigForwarder<Context>{
        std::vector<OptionHolder<IOption>> m_options;
        std::vector<OptionHolder<IOption>> m_groups;
        std::vector<OptionHolder<IOption>> m_positionals;
    public:
        ContextConfig config = ContextConfig(true);

        // Context() = default;
        explicit Context(const bool allowConfigUseDefault) : config(allowConfigUseDefault) {}

        Context(const Context&) = default;
        auto operator=(const Context&) -> Context& = default;

        Context(Context&&) noexcept = default;
        auto operator=(Context&&) noexcept -> Context& = default;

        template <typename T> requires detail::DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        [[nodiscard]] auto getFlagOption(std::string_view flag) -> IOption *;

        [[nodiscard]] auto getFlagOption(const FlagPath& flagPath) -> IOption *;

        [[nodiscard]] auto getOptionGroup(std::string_view flag) -> OptionGroup *;

        [[nodiscard]] auto getOptionGroup(const FlagPath& flagPath) -> OptionGroup *;

        [[nodiscard]] auto getPositional(size_t position) -> IsPositional *;

        template <typename T>
        auto getOptionDynamic(std::string_view flag) -> T *;

        [[nodiscard]] auto containsLocalFlag(std::string_view flag) const -> bool;

        template<typename ValueType>
        [[nodiscard]] auto getOptionValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        [[nodiscard]] auto getMultiValue(const FlagPath& flagPath) -> const Container&;


        template <typename ValueType, size_t Pos>
        [[nodiscard]] auto getPositionalValue() -> const ValueType&;

        template <typename ValueType, size_t Pos>
        [[nodiscard]] auto getPositionalValue(const FlagPath& groupPath) -> const ValueType&;

        [[nodiscard]] auto collectAllSetOptions() const -> OptionMap;

        [[nodiscard]] auto getOptions() -> std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getOptions() const -> const std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getGroups() -> std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getGroups() const -> const std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getPositionals() -> std::vector<OptionHolder<IOption>>&;

        [[nodiscard]] auto getPositionals() const -> const std::vector<OptionHolder<IOption>>&;

        auto validate(ErrorGroup& errorGroup) const -> void;

    private:
        [[nodiscard]] auto getConfigImpl() -> ContextConfig& override;

        [[nodiscard]] auto getConfigImpl() const -> const ContextConfig& override;

        [[nodiscard]] auto collectAllFlags() const -> std::vector<const Flag*>;

        auto resolveFlagGroup(const FlagPath& flagPath) -> OptionGroup *;

        auto collectAllSetOptions(OptionMap& map, const std::vector<Flag>& pathSoFar) const -> void;

        auto validate(const FlagPath& pathSoFar, ErrorGroup& validationErrors) const -> void;

        auto checkPrefixes(ErrorGroup& validationErrors, const std::string& groupSoFar) const -> void;
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
        const auto positionals = group->getContext().getPositionals();
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

inline auto resolveAllChildContextConfigs(Context *context) -> void { // NOLINT (misc-no-recursion)
    for (auto& optionHolder : context->getGroups()) {
        const auto group = dynamic_cast<OptionGroup *>(optionHolder.getPtr());
        auto& groupContext = group->getContext();
        groupContext.config = resolveContextConfig(context->config, groupContext.config);
        resolveAllChildContextConfigs(&groupContext);
    }
}

} // End namespace Argon::detail

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {

template<typename T> requires detail::DerivesFrom<T, IOption>
auto Context::addOption(T&& option) -> void {
    if constexpr (detail::DerivesFrom<T, IsPositional>) {
        m_positionals.emplace_back(std::forward<T>(option));
    } else if (dynamic_cast<OptionGroup *>(&option)) {
        m_groups.emplace_back(std::forward<T>(option));
    } else {
        m_options.emplace_back(std::forward<T>(option));
    }
}

inline auto Context::getFlagOption(const std::string_view flag) -> IOption * {
    const auto optIt = std::ranges::find_if(m_options, [&flag](const OptionHolder<IOption>& holder) {
        const auto iFlag = detail::getIFlag(holder);
        return iFlag->getFlag().containsFlag(flag);
    });
    if (optIt != m_options.end()) return optIt->getPtr();

    const auto groupIt = std::ranges::find_if(m_groups, [&flag](const OptionHolder<IOption>& holder) {
        const auto iFlag = detail::getIFlag(holder);
        return iFlag->getFlag().containsFlag(flag);
    });
    return groupIt == m_groups.end() ? nullptr : groupIt->getPtr();
}

inline auto Context::getFlagOption(const FlagPath& flagPath) -> IOption * {
    const auto group = resolveFlagGroup(flagPath);
    if (group == nullptr) {
        return getFlagOption(flagPath.flag);
    }
    return group->getOption(flagPath.flag);
}

inline auto Context::getOptionGroup(std::string_view flag) -> OptionGroup * {
    const auto it = std::ranges::find_if(m_groups, [&flag](const OptionHolder<IOption>& holder) {
        return detail::getIFlag(holder)->getFlag().containsFlag(flag);
    });

    return it == m_groups.end() ? nullptr : dynamic_cast<OptionGroup *>(it->getPtr());
}

inline auto Context::getOptionGroup(const FlagPath& flagPath) -> OptionGroup * {
    const auto group = resolveFlagGroup(flagPath);
    if (group == nullptr) {
        return getOptionGroup(flagPath.flag);
    }
    return group->getContext().getOptionGroup(flagPath.flag);
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
    const auto group = resolveFlagGroup(flagPath);
    const auto opt = group == nullptr ?
        getOptionDynamic<Option<ValueType>>(flagPath.flag) :
        group->getContext().getOptionDynamic<Option<ValueType>>(flagPath.flag);
    if (opt == nullptr) {
        throw InvalidFlagPathException(flagPath);
    }
    return opt->getValue();
}

template<typename Container>
auto Context::getMultiValue(const FlagPath& flagPath) -> const Container& {
    const auto group = resolveFlagGroup(flagPath);
    const auto opt = group == nullptr ?
        getOptionDynamic<MultiOption<Container>>(flagPath.flag) :
        group->getContext().getOptionDynamic<MultiOption<Container>>(flagPath.flag);
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
    const auto group = getOptionGroup(groupPath);
    if (group == nullptr) {
        return getPositionalValue<ValueType, Pos>();
    }
    return group->getContext().getPositionalValue<ValueType, Pos>();
}

inline auto Context::containsLocalFlag(const std::string_view flag) const -> bool {
    const auto allFlags = collectAllFlags();
    return std::ranges::any_of(allFlags, [&flag](const Flag* flagInList) {
        return flagInList->containsFlag(flag);
    });
}

inline auto Context::collectAllSetOptions() const -> OptionMap {
    OptionMap result;
    collectAllSetOptions(result, std::vector<Flag>());
    return result;
}

inline auto Context::getOptions() -> std::vector<OptionHolder<IOption>>& {
    return m_options;
}

inline auto Context::getOptions() const -> const std::vector<OptionHolder<IOption>>& {
    return m_options;
}

inline auto Context::getGroups() -> std::vector<OptionHolder<IOption>>& {
    return m_groups;
}

inline auto Context::getGroups() const -> const std::vector<OptionHolder<IOption>>& {
    return m_groups;
}

inline auto Context::getPositionals() -> std::vector<OptionHolder<IOption>>& {
    return m_positionals;
}

inline auto Context::getPositionals() const -> const std::vector<OptionHolder<IOption>>& {
    return m_positionals;
}

inline auto Context::collectAllSetOptions(OptionMap& map, //NOLINT (recursion)
                                          const std::vector<Flag>& pathSoFar) const -> void {
    for (const auto& holder : m_options) {
        const IOption *optPtr = holder.getPtr();
        auto *flagPtr = detail::getIFlag(holder);

        if (!optPtr->isSet()) continue;
        map[FlagPathWithAlias(pathSoFar, flagPtr->getFlag())] = optPtr;
    }

    for (const auto& holder : m_groups) {
        const auto group = dynamic_cast<const OptionGroup *>(holder.getPtr());
        if (group == nullptr) {
            assert(group != nullptr && "Non option group found in m_groups");
            continue;
        }

        auto newVec = pathSoFar;
        newVec.emplace_back(detail::getIFlag(holder)->getFlag());
        group->getContext().collectAllSetOptions(map, newVec);
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
    checkPrefixes(validationErrors, pathSoFar.getString());
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
                std::format(R"(Multiple flags found with the value of "{}")", flag),
                -1, ErrorType::Validation_DuplicateFlag);
        } else {
            validationErrors.addErrorMessage(std::format(
                R"(Multiple flags found with the value of "{}" within group "{}")", flag, pathSoFar.getString()),
                -1, ErrorType::Validation_DuplicateFlag);
        }
    }

    for (const auto& holder : m_groups) {
        const IOption& ref = holder.getRef();
        const auto& group = dynamic_cast<const OptionGroup&>(ref);
        FlagPath newPath = pathSoFar;
        newPath.extendPath(group.getFlag().mainFlag);
        group.getContext().validate(newPath, validationErrors);
    }
}

inline auto Context::checkPrefixes(ErrorGroup& validationErrors, const std::string& groupSoFar) const -> void {
    auto addErrorNoPrefix = [&](const std::string& flag) {
        if (groupSoFar.empty()) {
            validationErrors.addErrorMessage(
                std::format(R"(Flag "{}" does not start with a flag prefix)", flag),
                -1, ErrorType::Validation_NoPrefix);
        } else {
            validationErrors.addErrorMessage(
                std::format(R"(Flag "{}" inside group "{}" does not start with a flag prefix)", flag, groupSoFar),
                -1, ErrorType::Validation_NoPrefix);
        }
    };

    auto addErrorEmptyFlag = [&] {
        if (groupSoFar.empty()) {
            validationErrors.addErrorMessage("Empty flag found at top-level. Flag cannot be the empty string",
                        -1, ErrorType::Validation_EmptyFlag);
        } else {
            validationErrors.addErrorMessage(
                    std::format(R"(Empty flag found within group "{}". Flag cannot be the empty string)",
                        groupSoFar), -1, ErrorType::Validation_EmptyFlag);
        }
    };

    auto check = [&](const OptionHolder<IOption>& holder) {
        const auto flag = detail::getIFlag(holder);
        const auto& mainFlag = flag->getFlag().mainFlag;
        if (mainFlag.empty()) {
            addErrorEmptyFlag();
        } else if (!detail::startsWithAny(mainFlag, config.getFlagPrefixes())) {
            addErrorNoPrefix(mainFlag);
        }
        for (const auto& alias : flag->getFlag().aliases) {
            if (alias.empty()) {
                addErrorEmptyFlag();
            } else if (!detail::startsWithAny(alias, config.getFlagPrefixes())) {
                addErrorNoPrefix(alias);
            }
        }
    };

    for (const auto& holder : m_options) {
        check(holder);
    }
    for (const auto& holder : m_groups) {
        check(holder);
    }
}

inline auto Context::collectAllFlags() const -> std::vector<const Flag*> {
    std::vector<const Flag*> result;
    for (const auto& holder : m_options) {
        const auto iFlag = detail::getIFlag(holder);
        result.emplace_back(&iFlag->getFlag());
    }
    for (const auto& holder : m_groups) {
        const auto iFlag = detail::getIFlag(holder);
        result.emplace_back(&iFlag->getFlag());
    }
    return result;
}

inline auto Context::resolveFlagGroup(const FlagPath& flagPath) -> OptionGroup * {
    OptionGroup *group = nullptr;
    for (const auto& groupFlag : flagPath.groupPath) {
        auto& groups = group == nullptr ? m_groups : group->getContext().m_groups;
        auto it = std::ranges::find_if(groups, [&groupFlag](const OptionHolder<IOption>& holder) {
            return detail::getIFlag(holder)->getFlag().containsFlag(groupFlag);
        });

        if (it == groups.end()) {
            throw InvalidFlagPathException(flagPath);
        }
        group = dynamic_cast<OptionGroup *>(it->getPtr());
    }
    return group;
}

} // End namespace Argon

#endif // ARGON_CONTEXT_INCLUDE