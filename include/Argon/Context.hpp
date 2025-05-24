#pragma once

#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace Argon {
    class IOption;
    
    class Context {
        std::vector<std::unique_ptr<IOption>> m_options;
        std::vector<std::string> m_flags;
        std::string m_name;
        Context *m_parent = nullptr;
    public:
        Context() = default;
        explicit Context(std::string name);
        Context(const Context&);
        Context& operator=(const Context&);
        
        void addOption(const IOption& option);
        IOption *getOption(const std::string& flag);

        void setName(const std::string& name);
        [[nodiscard]] std::string getPath() const;

        template <typename T>
        T *getOptionDynamic(const std::string& flag);

        [[nodiscard]] bool containsLocalFlag(const std::string& flag) const;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <algorithm>

#include "Option.hpp" // NOLINT (misc-unused-include)

inline Argon::Context::Context(std::string name) : m_name(std::move(name)) {}

inline Argon::Context::Context(const Context& other) {
    for (const auto& option : other.m_options) {
        m_options.push_back(option->clone());
        if (auto *optionGroup = dynamic_cast<OptionGroup*>(m_options.back().get())) {
            optionGroup->get_context().m_parent = this;
        }
    }
    m_flags = other.m_flags;
    m_name = other.m_name;
    m_parent = other.m_parent;
}

inline Argon::Context& Argon::Context::operator=(const Context& other) {
    if (this == &other) {
        return *this;
    }

    m_options.clear();
    for (const auto& option : other.m_options) {
        m_options.push_back(option->clone());
        if (auto *optionGroup = dynamic_cast<OptionGroup*>(m_options.back().get())) {
            optionGroup->get_context().m_parent = this;
        }
    }
    m_flags = other.m_flags;
    m_name = other.m_name;
    m_parent = other.m_parent;
    return *this;
}

inline void Argon::Context::addOption(const IOption& option) {
    m_flags.insert(m_flags.end(), option.get_flags().begin(), option.get_flags().end());
    m_options.push_back(option.clone());
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(m_options.back().get())) {
        optionGroup->get_context().m_parent = this;
    }
}

inline Argon::IOption* Argon::Context::getOption(const std::string& flag) {
    const auto it = std::ranges::find_if(m_options, [&flag](const auto& option) {
       return std::ranges::contains(option->get_flags(), flag);
    });
    return it == m_options.end() ? nullptr : it->get();
}

inline void Argon::Context::setName(const std::string& name) {
    m_name = name;
}

inline std::string Argon::Context::getPath() const {
    if (m_parent == nullptr) {
        return "";
    }

    std::vector<const std::string*> names;

    const auto *current = this;
    while (current != nullptr) {
        if (!current->m_name.empty()) {
            names.push_back(&current->m_name);
        }
        current = current->m_parent;
    }

    std::ranges::reverse(names);
    return std::accumulate(std::next(names.begin()), names.end(), *(names[0]),
        [] (const std::string &name1, const std::string *name2) {
            return name1 + " > " + *name2;
    });
}

template <typename T>
T* Argon::Context::getOptionDynamic(const std::string& flag) {
    return dynamic_cast<T*>(getOption(flag));
}

inline bool Argon::Context::containsLocalFlag(const std::string& flag) const {
    return std::ranges::contains(m_flags, flag);
}
