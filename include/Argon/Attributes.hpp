#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <format>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Argon {
    class Context;

    class Constraints {
    public:
        auto require(std::string_view flag) -> Constraints&;

        auto mutuallyExclusive(std::string_view flag, std::initializer_list<std::string_view> exclusiveFlags) -> Constraints&;

        auto dependsOn(std::string_view flag, std::initializer_list<std::string_view> dependentFlags) -> Constraints&;

        auto validate(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void;

    private:
        std::set<std::string> m_requiredOptions;
        std::unordered_map<std::string, std::set<std::string>> m_mutuallyExclusiveOptions;
        std::unordered_map<std::string, std::set<std::string>> m_dependentFlags;

        void checkRequiredFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const;

        void checkMutuallyExclusive(Context& rootContext, std::vector<std::string>& errorMsgs) const;

        void checkDependentFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const;
    };
}

//---------------------------------------------------Implementations----------------------------------------------------

#include "Context.hpp"
#include "Option.hpp"

namespace Argon {

inline auto Constraints::require(std::string_view flag) -> Constraints& {
    m_requiredOptions.emplace(flag);
    return *this;
}

inline auto Constraints::mutuallyExclusive(const std::string_view flag,
                                           const std::initializer_list<std::string_view> exclusiveFlags) -> Constraints
    & {
    auto& flags = m_mutuallyExclusiveOptions[std::string(flag)];
    for (const auto& flagToAdd : exclusiveFlags) {
        flags.emplace(flagToAdd);
        m_mutuallyExclusiveOptions[std::string(flagToAdd)].emplace(flag);
    }
    return *this;
}

inline auto Constraints::dependsOn(const std::string_view flag,
                                   const std::initializer_list<std::string_view> dependentFlags) -> Constraints& {
    auto& flags = m_dependentFlags[std::string(flag)];
    for (const auto& flagToAdd : dependentFlags) {
        flags.emplace(flagToAdd);
    }
    return *this;
}

inline auto Constraints::validate(Context& rootContext, std::vector<std::string>& errorMsgs) const -> void {
    checkRequiredFlags      (rootContext, errorMsgs);
    checkMutuallyExclusive  (rootContext, errorMsgs);
    checkDependentFlags     (rootContext, errorMsgs);
}

inline void Constraints::checkRequiredFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const {
    for (const auto& requiredFlag : m_requiredOptions) {
        const auto opt = rootContext.getOption(requiredFlag);
        if (opt == nullptr) {
            std::cerr << "Option " << requiredFlag << " not found\n";
            return;
        }
        if (!opt->isSet()) {
            errorMsgs.emplace_back(std::format("Flag '{}' is a required flag and must be set", requiredFlag));
        }
    }
}

inline void Constraints::checkMutuallyExclusive(Context& rootContext, std::vector<std::string>& errorMsgs) const {
    std::vector<std::string_view> errorFlags;
    for (auto& [flag, exclusiveFlags] : m_mutuallyExclusiveOptions) {
        const auto opt = rootContext.getOption(flag);
        // Option doesn't exist in context
        if (opt == nullptr) {
            std::cerr << "Option " << flag << " not found\n";
            return;
        };

        // Option not specified
        if (!opt->isSet()) continue;

        // Check the exclusive flags list
        for (const auto& errorFlag : exclusiveFlags) {
            const auto errOpt = rootContext.getOption(errorFlag);
            if (errOpt == nullptr) {
                std::cerr << "Option " << errorFlag << " not found\n";
                return;
            }
            if (errOpt->isSet()) {
                errorFlags.push_back(errorFlag);
            }
        }
        if (!errorFlags.empty()) {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' is mutually exclusive with flags: ", flag);
            for (size_t i = 0; i < errorFlags.size(); i++) {
                msg += std::format("'{}'", errorFlags[i]);
                if (i != errorFlags.size() - 1) {
                  msg += ", ";
                }
            }
        }
    }
}

inline void Constraints::checkDependentFlags(Context& rootContext, std::vector<std::string>& errorMsgs) const {
    std::vector<std::string_view> errorFlags;
    for (auto& [flag, dependentFlags] : m_dependentFlags) {
        const auto opt = rootContext.getOption(flag);
        // Option doesn't exist in context
        if (opt == nullptr) {
            std::cerr << "Option " << flag << " not found\n";
            return;
        };

        // Option not specified
        if (!opt->isSet()) continue;

        // Check the dependent flags list
        for (const auto& errorFlag : dependentFlags) {
            const auto errOpt = rootContext.getOption(errorFlag);
            if (errOpt == nullptr) {
                std::cerr << "Option " << errorFlag << " not found\n";
                return;
            }
            if (!errOpt->isSet()) {
                errorFlags.push_back(errorFlag);
            }
        }
        if (!errorFlags.empty()) {
            auto& msg = errorMsgs.emplace_back();
            msg += std::format("Flag '{}' requires flags: ", flag);
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
