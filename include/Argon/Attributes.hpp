#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <format>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Flag.hpp"

namespace Argon {
    class IOption;
    class Context;
    using OptionMap = std::unordered_map<FlagPathWithAlias, IOption*>;

    class Constraints {
    public:
        auto require            (const FlagPath& flagPath) -> Constraints&;

        auto mutuallyExclusive  (const FlagPath& flagPath, std::initializer_list<FlagPath> exclusiveFlags) -> Constraints&;

        auto dependsOn          (const FlagPath& flagPath, std::initializer_list<FlagPath> dependentFlags) -> Constraints&;

        auto validate(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

    private:
        std::set<FlagPath> m_requiredFlags;
        std::unordered_map<FlagPath, std::set<FlagPath>> m_mutuallyExclusiveFlags;
        std::unordered_map<FlagPath, std::set<FlagPath>> m_dependentFlags;

        static auto checkMultiOptionStdArray    (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) -> void;

        auto checkRequiredFlags                 (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;

        auto checkMutuallyExclusive             (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;

        auto checkDependentFlags                (const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void;
    };
}

//---------------------------------------------------Implementations----------------------------------------------------

#include "Context.hpp"
#include "Option.hpp"

namespace Argon {

inline auto Constraints::require(const FlagPath& flagPath) -> Constraints& {
    m_requiredFlags.emplace(flagPath);
    return *this;
}

inline auto Constraints::mutuallyExclusive(const FlagPath& flagPath,
                                           const std::initializer_list<FlagPath> exclusiveFlags) -> Constraints& {
    auto& flags = m_mutuallyExclusiveFlags[flagPath];
    for (const auto& flagToAdd : exclusiveFlags) {
        flags.emplace(flagToAdd);
        m_mutuallyExclusiveFlags[flagToAdd].emplace(flagPath);
    }
    return *this;
}

inline auto Constraints::dependsOn(const FlagPath& flagPath,
                                   const std::initializer_list<FlagPath> dependentFlags) -> Constraints& {
    auto& flags = m_dependentFlags[flagPath];
    for (const auto& flagToAdd : dependentFlags) {
        flags.emplace(flagToAdd);
    }
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
        const auto multiOption = dynamic_cast<MultiOptionStdArrayBase*>(option);
        if (multiOption == nullptr) continue;

        if (!multiOption->isAtMaxCapacity()) {
            errorMsgs.emplace_back(std::format(
                "Flag '{}' must have exactly {} values specified",
                flag.getString(), multiOption->getMaxSize()));
        }
    }
}

inline auto Constraints::checkRequiredFlags(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void {
    for (const auto& requiredFlag : m_requiredFlags) {
        const FlagPathWithAlias *flag = containsFlag(setOptions, requiredFlag);
        if (flag == nullptr) {
            errorMsgs.emplace_back(std::format("Flag '{}' is a required flag and must be set", requiredFlag.getString()));
        }
    }
}

inline auto Constraints::checkMutuallyExclusive(const OptionMap& setOptions, std::vector<std::string>& errorMsgs) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flagToCheck, exclusiveFlags] : m_mutuallyExclusiveFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = containsFlag(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : exclusiveFlags) {
            const FlagPathWithAlias *errorFlagAlias = containsFlag(setOptions, errorFlag);
            // If error flag is set
            if (errorFlagAlias != nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (!errorFlags.empty()) {
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
    for (auto& [flagToCheck, dependentFlags] : m_dependentFlags) {
        errorFlags.clear();
        const FlagPathWithAlias *flag = containsFlag(setOptions, flagToCheck);
        // Flag to check is not set
        if (flag == nullptr) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : dependentFlags) {
            const FlagPathWithAlias *errorFlagAlias = containsFlag(setOptions, errorFlag);
            // If error flag is not set
            if (errorFlagAlias == nullptr) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (!errorFlags.empty()) {
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
} // End namespace Argon

#endif