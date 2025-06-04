#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <format>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Flag.hpp"

namespace Argon {
    class Context;

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

        auto checkMultiOptionStdArray   (Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

        auto checkRequiredFlags         (Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

        auto checkMutuallyExclusive     (Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

        auto checkDependentFlags        (Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;
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
    auto res = rootContext.collectAllSetOptions();

    checkRequiredFlags      (rootContext, errorMsgs);
    checkMutuallyExclusive  (rootContext, errorMsgs);
    checkDependentFlags     (rootContext, errorMsgs);
}

inline auto Constraints::checkRequiredFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void {
    for (const auto& requiredFlag : m_requiredFlags) {
        const auto opt = rootContext.getOption(requiredFlag);
        if (opt == nullptr) {
            throw InvalidFlagPathException(requiredFlag);
        }
        if (!opt->isSet()) {
            errorMsgs.emplace_back(std::format("Flag '{}' is a required flag and must be set", requiredFlag.getString()));
        }
    }
}

inline auto Constraints::checkMutuallyExclusive(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flag, exclusiveFlags] : m_mutuallyExclusiveFlags) {
        const auto opt = rootContext.getOption(flag);
        // Option doesn't exist in context
        if (opt == nullptr) {
            throw InvalidFlagPathException(flag);
        };

        // Option not specified
        if (!opt->isSet()) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : exclusiveFlags) {
            const auto errOpt = rootContext.getOption(errorFlag);
            if (errOpt == nullptr) {
                throw InvalidFlagPathException(errorFlag);
            }
            if (errOpt->isSet()) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (!errorFlags.empty()) {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' is mutually exclusive with flags: ", flag.getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format("'{}'", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                  msg += ", ";
                }
            }
        }
    }
}

inline auto Constraints::checkDependentFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void {
    std::vector<std::string> errorFlags;
    for (auto& [flag, dependentFlags] : m_dependentFlags) {
        const auto opt = rootContext.getOption(flag);
        // Option doesn't exist in context
        if (opt == nullptr) {
            throw InvalidFlagPathException(flag);
        };

        // Option not specified
        if (!opt->isSet()) continue;

        // Check the dependent flags list
        for (const auto& errorFlag : dependentFlags) {
            const auto errOpt = rootContext.getOption(errorFlag);
            if (errOpt == nullptr) {
                throw InvalidFlagPathException(errorFlag);
            }
            if (!errOpt->isSet()) {
                errorFlags.push_back(errorFlag.getString());
            }
        }
        if (!errorFlags.empty()) {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' requires flags: ", flag.getString());
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format("'{}'", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                    msg += ", ";
                }
            }
            msg += " to be set";
        }
    }
}
} // End namespace Argon

#endif