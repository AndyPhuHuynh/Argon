#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include "Flag.hpp"

namespace Argon {
    class IOption;
    class Context;
    using OptionMap = std::unordered_map<FlagPathWithAlias, const IOption*>;
    using GenerateConstraintErrorMsgFn = std::function<std::string(std::vector<std::string>)>;

    struct Requirement {
        FlagPath flagPath;
        std::string errorMsg;

        Requirement() = default;
        explicit Requirement(const FlagPath& flagPath);
        Requirement(const FlagPath& flagPath, std::string_view errorMsg);

    };

    struct MutuallyExclusive {
        FlagPath flagPath;
        std::vector<FlagPath> exclusives;
        std::string errorMsg;
        GenerateConstraintErrorMsgFn genErrorMsg;

        MutuallyExclusive() = default;
        MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives);
        MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives, std::string_view errorMsg);
        MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives,
            const GenerateConstraintErrorMsgFn& genErrorMsg);
    };

    struct DependentOptions {
        FlagPath flagPath;
        std::vector<FlagPath> exclusives;
        std::string errorMsg;
        GenerateConstraintErrorMsgFn genErrorMsg;

        DependentOptions() = default;
        DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives);
        DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives, std::string_view errorMsg);
        DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives,
            const GenerateConstraintErrorMsgFn& genErrorMsg);
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

        auto validate(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

    private:
        std::vector<Requirement> m_requiredFlags;
        std::vector<MutuallyExclusive> m_mutuallyExclusiveFlags;
        std::vector<DependentOptions> m_dependentFlags;

        static auto checkMultiOptionStdArray    (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) -> void;

        auto checkRequiredFlags                 (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;

        auto checkMutuallyExclusive             (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;

        auto checkDependentFlags                (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;
    };
}

//---------------------------------------------------Implementations----------------------------------------------------

#include "Argon/Context.hpp"
#include "Argon/Options/Option.hpp"

namespace Argon {
inline Requirement::Requirement(const FlagPath& flagPath) : flagPath(flagPath) {}

inline Requirement::Requirement(const FlagPath& flagPath, const std::string_view errorMsg)
    : flagPath(flagPath), errorMsg(errorMsg) {}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives)
    : flagPath(flagPath), exclusives(exclusives) {}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives,
    const std::string_view errorMsg) : flagPath(flagPath), exclusives(exclusives), errorMsg(errorMsg) {}

inline MutuallyExclusive::MutuallyExclusive(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives,
    const GenerateConstraintErrorMsgFn& genErrorMsg)
    : flagPath(flagPath), exclusives(exclusives), genErrorMsg(genErrorMsg) {}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives)
    : flagPath(flagPath), exclusives(exclusives) {}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, const std::initializer_list<FlagPath> exclusives,
    const std::string_view errorMsg) : flagPath(flagPath), exclusives(exclusives), errorMsg(errorMsg) {}

inline DependentOptions::DependentOptions(const FlagPath& flagPath, std::initializer_list<FlagPath> exclusives,
    const GenerateConstraintErrorMsgFn& genErrorMsg)
    : flagPath(flagPath), exclusives(exclusives), genErrorMsg(genErrorMsg) {}


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

inline auto Constraints::validate(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void {
    const auto setOptions = rootContext.collectAllSetOptions();

    checkMultiOptionStdArray(setOptions, errorMsgs);
    checkRequiredFlags      (setOptions, errorMsgs);
    checkMutuallyExclusive  (setOptions, errorMsgs);
    checkDependentFlags     (setOptions, errorMsgs);
}

inline auto Constraints::checkMultiOptionStdArray(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) -> void {
    for (const auto& [flag, option] : setOptions) {
        const auto multiOption = dynamic_cast<const IArrayCapacity*>(option);
        if (multiOption == nullptr) continue;

        if (!multiOption->isAtMaxCapacity()) {
            errorMsgs.emplace_back(std::format(
                "Flag '{}' must have exactly {} values specified",
                flag.getString(), multiOption->getMaxSize()));
        }
    }
}

inline auto Constraints::checkRequiredFlags(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void {
    for (const auto& requirement : m_requiredFlags) {
        if (containsFlagPath(setOptions, requirement.flagPath)) {
            continue;
        }
        if (requirement.errorMsg.empty()) {
            errorMsgs.emplace_back(std::format(
                "Flag '{}' is a required flag and must be set",
                requirement.flagPath.getString()));
        } else {
            errorMsgs.emplace_back(requirement.errorMsg);
        }
    }
}

inline auto Constraints::checkMutuallyExclusive(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flagToCheck, exclusiveFlags, customErr, genMsg] : m_mutuallyExclusiveFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = containsFlagPath(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : exclusiveFlags) {
            const FlagPathWithAlias *errorFlagAlias = containsFlagPath(setOptions, errorFlag);
            // If error flag is set
            if (errorFlagAlias != nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (errorFlags.empty()) {
            continue;
        }
        if (!customErr.empty()) {
            errorMsgs.emplace_back(customErr);
        } else if (genMsg != nullptr) {
            errorMsgs.emplace_back(genMsg(errorFlags));
        } else {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' is mutually exclusive with flags: ", flag->getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format("'{}'", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                    msg += ", ";
                }
            }
        }
    }
}

inline auto Constraints::checkDependentFlags(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flagToCheck, dependentFlags, customErr, genMsg] : m_dependentFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = containsFlagPath(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : dependentFlags) {
            const FlagPathWithAlias *errorFlagAlias = containsFlagPath(setOptions, errorFlag);
            // If error flag is not set
            if (errorFlagAlias == nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (errorFlags.empty()) {
            continue;
        }
        if (!customErr.empty()) {
            errorMsgs.emplace_back(customErr);
        } else if (genMsg != nullptr) {
            errorMsgs.emplace_back(genMsg(errorFlags));
        } else {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' must be set with flags: ", flag->getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format("'{}'", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                    msg += ", ";
                }
            }
        }
    }
}
} // End namespace Argon

#endif