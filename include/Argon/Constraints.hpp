#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <format>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Flag.hpp"

namespace Argon {

class IOption;
class Context;
using OptionMap = std::unordered_map<FlagPathWithAlias, const IOption*>;
using GenerateConstraintErrorMsgFn = std::function<std::string(std::vector<std::string>)>;

class Parser;
namespace detail {
    class ConstraintsQuery;
}

struct Requirement {
    FlagPathWithAlias flagPath;
    std::string errorMsg;

    Requirement() = default;
    explicit Requirement(const FlagPath& flagPath);
    Requirement(const FlagPath& flagPath, std::string_view errorMsg);
};

struct MutuallyExclusive {
    FlagPathWithAlias flagPath;
    std::vector<FlagPathWithAlias> exclusives;
    std::string errorMsg;
    GenerateConstraintErrorMsgFn genErrorMsg;

    MutuallyExclusive() = default;
    MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives);
    MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives, std::string_view errorMsg);
    MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives,
        GenerateConstraintErrorMsgFn  genErrorMsg);
};

struct DependentOptions {
    FlagPathWithAlias flagPath;
    std::vector<FlagPathWithAlias> dependents;
    std::string errorMsg;
    GenerateConstraintErrorMsgFn genErrorMsg;

    DependentOptions() = default;
    DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> dependents);
    DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> dependents, std::string_view errorMsg);
    DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> dependents,
        GenerateConstraintErrorMsgFn  genErrorMsg);
};

class Constraints {
public:
    auto require(const FlagPath& flagPath) -> Constraints&;
    auto require(const FlagPath& flagPath, std::string_view errorMsg) -> Constraints&;

    auto mutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags) -> Constraints&;
    auto mutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags,
        std::string_view errorMsg) -> Constraints&;
    auto mutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags,
        const GenerateConstraintErrorMsgFn& errorFn) -> Constraints&;

    auto dependsOn(const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags) -> Constraints&;
    auto dependsOn(const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags,
                   std::string_view errorMsg) -> Constraints&;
    auto dependsOn(const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags,
        const GenerateConstraintErrorMsgFn& errorFn) -> Constraints&;

    auto validateSetup(const Context& rootContext, ErrorGroup& validationErrors) -> void;

    auto validate(const Context& rootContext, ErrorGroup& constraintErrors) const -> void;

private:
    std::vector<Requirement> m_requiredFlags;
    std::vector<MutuallyExclusive> m_mutuallyExclusiveFlags;
    std::vector<DependentOptions> m_dependentFlags;

    friend class Parser;
    friend class detail::ConstraintsQuery;
    Constraints() = default;

    auto resolveAliases(const std::vector<FlagPathWithAlias>& allFlags, ErrorGroup& validationErrors) -> void;

    auto validateRequirementSetup(ErrorGroup& validationErrors) const -> void;

    auto validateMutuallyExclusiveSetup(ErrorGroup& validationErrors) const -> void;

    auto validateDependenciesSetup(ErrorGroup& validationErrors) const -> void;

    static auto checkMultiOptionStdArray    (const OptionMap& setOptions, ErrorGroup& constraintErrors) -> void;

    auto checkRequiredFlags                 (const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void;

    auto checkMutuallyExclusive             (const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void;

    auto checkDependentFlags                (const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void;
};

} // End namespace Argon

namespace Argon::detail {

class ConstraintsQuery {
public:
    static auto containsRequirement(const Constraints& constraints, const FlagPath& flagPath) -> bool ;

    static auto containsMutuallyExclusive(const Constraints& constraints, const FlagPath& flagPath) -> std::optional<MutuallyExclusive>;

    static auto containsDependentOption(const Constraints& constraints, const FlagPath& flagPath) -> std::optional<DependentOptions>;
};

} // End namespace Argon::detail

//---------------------------------------------------Free Functions-----------------------------------------------------

#include "Argon/Context.hpp"
#include "Argon/Options/Option.hpp"

namespace Argon::detail {

inline auto containsFlagPath(const OptionMap& map, const FlagPath& flagPath) -> const FlagPathWithAlias * {
    for (const auto& flagWithAlias : map | std::views::keys) {
        if (isAlias(flagWithAlias, flagPath)) return &flagWithAlias;
    }
    return nullptr;
}

inline auto containsFlagPath(const OptionMap& map, const FlagPathWithAlias& flagPath) -> const FlagPathWithAlias * {
    for (const auto& flag : map | std::views::keys) {
        if (isAlias(flag, flagPath)) {
            return &flag;
        }
    }
    return nullptr;
}

} // End namespace Argon::detail

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {
inline Requirement::Requirement(const FlagPath& flagPath) : flagPath(flagPath) {}

inline Requirement::Requirement(const FlagPath& flagPath, const std::string_view errorMsg)
    : flagPath(flagPath), errorMsg(errorMsg) {}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives)
    : flagPath(flagPath) {
    for (const auto& flag : exclusives) {
        this->exclusives.emplace_back(flag);
    }
}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives,
    const std::string_view errorMsg) : flagPath(flagPath), errorMsg(errorMsg) {
    for (const auto& flag : exclusives) {
        this->exclusives.emplace_back(flag);
    }
}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives,
    GenerateConstraintErrorMsgFn  genErrorMsg)
    : flagPath(flagPath), genErrorMsg(std::move(genErrorMsg)) {
    for (const auto& flag : exclusives) {
        this->exclusives.emplace_back(flag);
    }
}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, const std::initializer_list<FlagPath> dependents)
    : flagPath(flagPath) {
    for (const auto& flag: dependents) {
        this->dependents.emplace_back(flag);
    }
}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, const std::initializer_list<FlagPath> dependents,
    const std::string_view errorMsg) : flagPath(flagPath), errorMsg(errorMsg) {
    for (const auto& flag: dependents) {
        this->dependents.emplace_back(flag);
    }
}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, const std::initializer_list<FlagPath> dependents,
    GenerateConstraintErrorMsgFn  genErrorMsg)
    : flagPath(flagPath), genErrorMsg(std::move(genErrorMsg)) {
    for (const auto& flag: dependents) {
        this->dependents.emplace_back(flag);
    }
}

inline auto Constraints::require(const FlagPath& flagPath) -> Constraints& {
    m_requiredFlags.emplace_back(flagPath);
    return *this;
}

inline auto Constraints::require(const FlagPath& flagPath, std::string_view errorMsg) -> Constraints& {
    m_requiredFlags.emplace_back(flagPath, errorMsg);
    return *this;
}

inline auto Constraints::mutuallyExclusive(const FlagPath& flagPath,
    const std::initializer_list<FlagPath> exclusiveFlags) -> Constraints& {
    m_mutuallyExclusiveFlags.emplace_back(flagPath, exclusiveFlags);
    return *this;
}

inline auto Constraints::mutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags,
    std::string_view errorMsg) -> Constraints& {
    m_mutuallyExclusiveFlags.emplace_back(flagPath, exclusiveFlags, errorMsg);
    return *this;
}

inline auto Constraints::mutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags,
    const GenerateConstraintErrorMsgFn& errorFn) -> Constraints& {
    m_mutuallyExclusiveFlags.emplace_back(flagPath, exclusiveFlags, errorFn);
    return *this;
}

inline auto Constraints::dependsOn(const FlagPath& flagPath,
    const std::initializer_list<FlagPath> dependentFlags) -> Constraints& {
    m_dependentFlags.emplace_back(flagPath, dependentFlags);
    return *this;
}

inline auto Constraints::dependsOn(const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags,
    std::string_view errorMsg) -> Constraints& {
    m_dependentFlags.emplace_back(flagPath, dependentFlags, errorMsg);
    return *this;
}

inline auto Constraints::dependsOn(const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags,
    const GenerateConstraintErrorMsgFn& errorFn) -> Constraints& {
    m_dependentFlags.emplace_back(flagPath, dependentFlags, errorFn);
    return *this;
}

inline auto Constraints::validateSetup(const Context& rootContext, ErrorGroup& validationErrors) -> void {
    resolveAliases(rootContext.collectAllFlagsRecursive(), validationErrors);
    validateRequirementSetup(validationErrors);
    validateMutuallyExclusiveSetup(validationErrors);
    validateDependenciesSetup(validationErrors);
}

inline auto Constraints::validate(const Context& rootContext, ErrorGroup& constraintErrors) const -> void {
    const auto setOptions = rootContext.collectAllSetOptions();
    checkMultiOptionStdArray(setOptions, constraintErrors);
    checkRequiredFlags      (setOptions, constraintErrors);
    checkMutuallyExclusive  (setOptions, constraintErrors);
    checkDependentFlags     (setOptions, constraintErrors);
}

inline auto Constraints::resolveAliases(const std::vector<FlagPathWithAlias>& allFlags, ErrorGroup& validationErrors) -> void {
    auto resolve = [&allFlags, &validationErrors](FlagPathWithAlias& flagToResolve) {
        for (const auto& flagPath : allFlags) {
            if (isAlias(flagToResolve, flagPath)) {
                flagToResolve = flagPath;
                return;
            }
        }
        validationErrors.addErrorMessage(
            std::format(R"(Flag passed into constraint "{}" does not exist.)", flagToResolve.getString()),
            -1, ErrorType::Validation_FlagDoesNotExist);
    };
    for (auto& requirement : m_requiredFlags) {
        resolve(requirement.flagPath);
    }
    for (auto& me : m_mutuallyExclusiveFlags) {
        resolve(me.flagPath);
        for (auto& flag : me.exclusives) {
            resolve(flag);
        }
    }
    for (auto& dependent : m_dependentFlags) {
        resolve(dependent.flagPath);
        for (auto& flag : dependent.dependents) {
            resolve(flag);
        }
    }
}

inline auto Constraints::validateRequirementSetup(ErrorGroup& validationErrors) const -> void {
    std::vector<FlagPathWithAlias> duplicatedFlags;
    for (size_t i = 0; i < m_requiredFlags.size(); i++) {
        for (size_t j = i + 1; j < m_requiredFlags.size(); j++) {
            if (m_requiredFlags[i].flagPath == m_requiredFlags[j].flagPath &&
                !std::ranges::contains(duplicatedFlags, m_requiredFlags[j].flagPath)) {
                duplicatedFlags.push_back(m_requiredFlags[i].flagPath);
                validationErrors.addErrorMessage(
                    std::format(R"(Flag "{}" was specified as a duplicate requirement.)",
                        m_requiredFlags[i].flagPath.getString()), -1, ErrorType::Validation_DuplicateRequirement);
            }
        }
    }
}

inline auto Constraints::validateMutuallyExclusiveSetup(ErrorGroup& validationErrors) const -> void {
    std::vector<FlagPathWithAlias> alreadyErrored;
    auto validateSelfExclusion = [&](const MutuallyExclusive& me) {
        for (const auto& flag : me.exclusives) {
            if (me.flagPath == flag && !std::ranges::contains(alreadyErrored, flag)) {
                alreadyErrored.push_back(flag);
                validationErrors.addErrorMessage(
                    std::format(R"(Flag "{}" was declare mutually exclusive with itself.)", me.flagPath.getString()),
                    -1, ErrorType::Validation_MutualExclusionCycle);
            }
        }
    };

    for (const auto& me : m_mutuallyExclusiveFlags) {
        validateSelfExclusion(me);
    }
}

inline auto Constraints::validateDependenciesSetup(ErrorGroup& validationErrors) const -> void {
    std::vector<FlagPathWithAlias> alreadyErrored;
    auto validateSelfExclusion = [&](const DependentOptions& dependent) {
        for (const auto& flag : dependent.dependents) {
            if (dependent.flagPath == flag && !std::ranges::contains(alreadyErrored, flag)) {
                alreadyErrored.push_back(flag);
                validationErrors.addErrorMessage(
                    std::format(R"(Flag "{}" was declare dependent with itself.)", dependent.flagPath.getString()),
                    -1, ErrorType::Validation_DependentCycle);
            }
        }
    };

    for (const auto& dependent : m_dependentFlags) {
        validateSelfExclusion(dependent);
    }
}

inline auto Constraints::checkMultiOptionStdArray(const OptionMap& setOptions, ErrorGroup& constraintErrors) -> void {
    for (const auto& [flag, option] : setOptions) {
        const auto multiOption = dynamic_cast<const IArrayCapacity*>(option);
        if (multiOption == nullptr) continue;

        if (!multiOption->isAtMaxCapacity()) {
            constraintErrors.addErrorMessage(
                std::format(
                    "Flag '{}' must have exactly {} values specified",
                    flag.getString(), multiOption->getMaxSize()),
                -1, ErrorType::Constraint_MultiOptionCount);
        }
    }
}

inline auto Constraints::checkRequiredFlags(const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void {
    for (const auto& requirement : m_requiredFlags) {
        if (detail::containsFlagPath(setOptions, requirement.flagPath)) {
            continue;
        }
        if (requirement.errorMsg.empty()) {
            constraintErrors.addErrorMessage(
                std::format(R"(Flag "{}" is a required flag and must be set)", requirement.flagPath.getString()),
                -1, ErrorType::Constraint_RequiredFlag);
        } else {
            constraintErrors.addErrorMessage(requirement.errorMsg, -1, ErrorType::Constraint_RequiredFlag);
        }
    }
}

inline auto Constraints::checkMutuallyExclusive(const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flagToCheck, exclusiveFlags, customErr, genMsg] : m_mutuallyExclusiveFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = detail::containsFlagPath(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : exclusiveFlags) {
            // If error flag is set
            if (const FlagPathWithAlias *errorFlagAlias = detail::containsFlagPath(setOptions, errorFlag);
                errorFlagAlias != nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (errorFlags.empty()) {
            continue;
        }
        if (!customErr.empty()) {
            constraintErrors.addErrorMessage(customErr, -1, ErrorType::Constraint_MutuallyExclusive);
        } else if (genMsg != nullptr) {
            constraintErrors.addErrorMessage(genMsg(errorFlags), -1, ErrorType::Constraint_MutuallyExclusive);
        } else {
            std::string msg = std::format(R"(Flag "{}" is mutually exclusive with flags: )", flag->getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format(R"("{}")", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                    msg += ", ";
                }
            }
            constraintErrors.addErrorMessage(msg, -1, ErrorType::Constraint_MutuallyExclusive);
        }
    }
}

inline auto Constraints::checkDependentFlags(const OptionMap& setOptions, ErrorGroup& constraintErrors) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flagToCheck, dependentFlags, customErr, genMsg] : m_dependentFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = detail::containsFlagPath(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : dependentFlags) {
            // If error flag is not set
            if (const FlagPathWithAlias *errorFlagAlias = detail::containsFlagPath(setOptions, errorFlag);
                errorFlagAlias == nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (errorFlags.empty()) {
            continue;
        }
        if (!customErr.empty()) {
            constraintErrors.addErrorMessage(customErr, -1, ErrorType::Constraint_DependentOption);
        } else if (genMsg != nullptr) {
            constraintErrors.addErrorMessage(genMsg(errorFlags), -1, ErrorType::Constraint_DependentOption);
        } else {
            std::string msg = std::format(R"(Flag "{}" must be set with flags: )", flag->getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format(R"("{}")", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                    msg += ", ";
                }
            }
            constraintErrors.addErrorMessage(msg, -1, ErrorType::Constraint_DependentOption);
        }
    }
}
} // End namespace Argon

namespace Argon::detail {

inline auto ConstraintsQuery::containsRequirement(const Constraints& constraints, const FlagPath& flagPath) -> bool {
    return std::ranges::any_of(constraints.m_requiredFlags, [&](const Requirement& req) {
        return isAlias(req.flagPath, flagPath);
    });
}

inline auto ConstraintsQuery::containsMutuallyExclusive(
    const Constraints& constraints, const FlagPath& flagPath
) -> std::optional<MutuallyExclusive> {
    const auto it = std::ranges::find_if(constraints.m_mutuallyExclusiveFlags, [&](const MutuallyExclusive& me) {
        return isAlias(me.flagPath, flagPath);
    });
    return it == constraints.m_mutuallyExclusiveFlags.end() ? std::nullopt : std::optional(*it);
}

inline auto ConstraintsQuery::containsDependentOption(
    const Constraints& constraints, const FlagPath& flagPath
) -> std::optional<DependentOptions> {
    const auto it = std::ranges::find_if(constraints.m_dependentFlags, [&](const DependentOptions& dep) {
            return isAlias(dep.flagPath, flagPath);
        });
    return it == constraints.m_dependentFlags.end() ? std::nullopt : std::optional(*it);
}

} // End namespace Argon::detail

#endif