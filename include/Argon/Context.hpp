#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Argon {
    class IOption;
    
    class Context {
        std::vector<std::unique_ptr<IOption>> m_options;
        std::vector<std::string> m_flags;

    public:
        Context() = default;
        Context(const Context&);
        Context& operator=(const Context&);
        
        void addOption(const IOption& option);
        IOption *getOption(const std::string& flag);

        template <typename T>
        T *getOptionDynamic(const std::string& flag);

        [[nodiscard]] bool containsLocalFlag(const std::string& flag) const;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include "Argon/Context.hpp"

#include <algorithm>

#include "Option.hpp" // NOLINT (misc-unused-include)

inline Argon::Context::Context(const Context& other) {
    for (const auto& option : other.m_options) {
        m_options.push_back(option->clone());
    }
    m_flags = other.m_flags;
}

inline Argon::Context& Argon::Context::operator=(const Context& other) {
    m_options.clear();
    for (const auto& option : other.m_options) {
        m_options.push_back(option->clone());
    }
    m_flags = other.m_flags;
    return *this;
}

inline void Argon::Context::addOption(const IOption& option) {
    m_flags.insert(m_flags.end(), option.get_flags().begin(), option.get_flags().end());
    m_options.push_back(option.clone());
}

inline Argon::IOption* Argon::Context::getOption(const std::string& flag) {
    const auto it = std::ranges::find_if(m_options, [&flag](const auto& option) {
       return std::ranges::contains(option->get_flags(), flag);
    });
    return it == m_options.end() ? nullptr : it->get();
}

template <typename T>
T* Argon::Context::getOptionDynamic(const std::string& flag) {
    return dynamic_cast<T*>(getOption(flag));
}

inline bool Argon::Context::containsLocalFlag(const std::string& flag) const {
    return std::ranges::contains(m_flags, flag);
}
