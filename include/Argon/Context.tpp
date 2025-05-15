#pragma once
#include "Argon/Context.hpp"

#include <algorithm>

#include "Option.hpp"

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

inline bool Argon::Context::containsLocalFlag(const std::string& flag) const {
    return std::ranges::contains(m_flags, flag);
}
