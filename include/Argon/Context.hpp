#pragma once

#include <memory>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "Traits.hpp"

namespace Argon {
    class IOption;
    using OptionPtr = std::variant<IOption*, std::unique_ptr<IOption>>;

    class Context {
        std::vector<OptionPtr> m_options;
        std::vector<std::string> m_flags;
        std::string m_name;
        Context *m_parent = nullptr;
    public:
        Context() = default;
        explicit Context(std::string name);
        Context(const Context&);
        Context& operator=(const Context&);
        
        auto addOption(IOption& option) -> void;
        auto addOption(IOption&& option) -> void;

        auto getOption(const std::string& flag) -> IOption *;

        auto setName(const std::string& name) -> void;
        [[nodiscard]] auto getPath() const -> std::string;

        template <typename T>
        auto getOptionDynamic(const std::string& flag) -> T*;

        [[nodiscard]] auto containsLocalFlag(const std::string& flag) const -> bool;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
    inline auto getRawPointer(const OptionPtr& optPtr) -> IOption* {
        return std::visit([]<typename T>(const T& opt) -> IOption* {
            if constexpr (std::is_same_v<T, IOption*>) {
                return opt;
            } else if constexpr (std::is_same_v<T, std::unique_ptr<IOption>>) {
                return opt.get();
            } else {
                static_assert(always_false<T>, "Unhandled type in getRawPointer");
                return nullptr;
            }
        }, optPtr);
    }
}

//---------------------------------------------------Implementations----------------------------------------------------

#include <algorithm>

#include "Option.hpp" // NOLINT (misc-unused-include)

inline Argon::Context::Context(std::string name) : m_name(std::move(name)) {}

inline Argon::Context::Context(const Context& other) {
    for (const auto& option : other.m_options) {
        std::visit([&]<typename T>(const T& opt) -> void {
            if constexpr (std::is_same_v<T, IOption*>) {
                m_options.emplace_back(opt);
            } else {
                m_options.emplace_back(opt->clone());
            }
        }, option);

        if (auto *optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(m_options.back()))) {
            optionGroup->getContext().m_parent = this;
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
        std::visit([&]<typename T>(const T& opt) -> void {
            if constexpr (std::is_same_v<T, IOption*>) {
                m_options.emplace_back(opt);
            } else {
                m_options.emplace_back(opt->clone());
            }
        }, option);

        if (auto *optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(m_options.back()))) {
            optionGroup->getContext().m_parent = this;
        }
    }
    m_flags = other.m_flags;
    m_name = other.m_name;
    m_parent = other.m_parent;
    return *this;
}

inline void Argon::Context::addOption(IOption& option) {
    m_flags.insert(m_flags.end(), option.getFlags().begin(), option.getFlags().end());
    m_options.emplace_back(&option);
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(&option)) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Argon::Context::addOption(IOption&& option) -> void {
    m_flags.insert(m_flags.end(), option.getFlags().begin(), option.getFlags().end());
    const auto& newOpt = m_options.emplace_back(option.clone());
    if (const auto optionGroup = dynamic_cast<OptionGroup*>(getRawPointer(newOpt))) {
        optionGroup->getContext().m_parent = this;
    }
}

inline auto Argon::Context::getOption(const std::string& flag) -> IOption* {
    const auto it = std::ranges::find_if(m_options, [&flag](const OptionPtr& option) {
        return std::visit([&flag]<typename T>(const T& opt) -> bool {
            return std::ranges::contains(opt->getFlags(), flag);
        }, option);
    });

    return it == m_options.end() ? nullptr : getRawPointer(*it);

}

inline auto Argon::Context::setName(const std::string& name) -> void {
    m_name = name;
}

inline auto Argon::Context::getPath() const -> std::string {
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
auto Argon::Context::getOptionDynamic(const std::string& flag) -> T* {
    return dynamic_cast<T*>(getOption(flag));
}

inline auto Argon::Context::containsLocalFlag(const std::string& flag) const -> bool {
    return std::ranges::contains(m_flags, flag);
}
