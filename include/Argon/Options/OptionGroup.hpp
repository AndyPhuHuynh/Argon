#ifndef ARGON_OPTION_GROUP_INCLUDE
#define ARGON_OPTION_GROUP_INCLUDE

#include "memory"

#include "Argon/Flag.hpp"
#include "Argon/Options/OptionComponent.hpp"
#include "Argon/Traits.hpp"

namespace Argon {
class Context;

class OptionGroup
    : public detail::ContextConfigForwarder<OptionGroup>,
      public HasFlag<OptionGroup>,
      public OptionComponent<OptionGroup> {
    std::unique_ptr<Context> m_context = std::make_unique<Context>(true);
public:
    OptionGroup() = default;

    OptionGroup(const OptionGroup&);
    auto operator=(const OptionGroup&) -> OptionGroup&;

    OptionGroup(OptionGroup&&) = default;
    auto operator=(OptionGroup&&) -> OptionGroup& = default;

    template <typename T> requires detail::DerivesFrom<T, IOption>
    auto operator+(T&& other) & -> OptionGroup&;

    template <typename T> requires detail::DerivesFrom<T, IOption>
    auto operator+(T&& other) && -> OptionGroup;

    template <typename T> requires detail::DerivesFrom<T, IOption>
    auto addOption(T&& option) -> void;

    [[nodiscard]] auto getOption(std::string_view flag) -> IOption *;

    [[nodiscard]] auto getContext() -> Context&;

    [[nodiscard]] auto getContext() const -> const Context&;
private:
    [[nodiscard]] auto getConfigImpl() -> ContextConfig& override;

    [[nodiscard]] auto getConfigImpl() const -> const ContextConfig& override;
};

} // End namespace Argon

//---------------------------------------------------Implementations----------------------------------------------------


#include "Argon/Context.hpp" // NOLINT (misc-unused-include)

namespace Argon {

inline OptionGroup::OptionGroup(const OptionGroup &other)
    : HasFlag(other), OptionComponent(other) {
    m_context = std::make_unique<Context>(*other.m_context);
}

inline auto OptionGroup::operator=(const OptionGroup& other) -> OptionGroup& {
    if (this == &other) {
        return *this;
    }
    HasFlag::operator=(other);
    OptionComponent::operator=(other);
    m_context = std::make_unique<Context>(*other.m_context);
    return *this;
}

template<typename T> requires detail::DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) & -> OptionGroup& {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template <typename T> requires detail::DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) && -> OptionGroup {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template<typename T> requires detail::DerivesFrom<T, IOption>
auto OptionGroup::addOption(T&& option) -> void {
    m_context->addOption(std::forward<T>(option));
}

inline auto OptionGroup::getOption(std::string_view flag) -> IOption* { //NOLINT (function is not const)
    return m_context->getFlagOption(flag);
}

inline auto OptionGroup::getContext() -> Context& { //NOLINT (function is not const)
    return *m_context;
}

inline auto OptionGroup::getContext() const -> const Context& {
    return *m_context;
}

inline auto OptionGroup::getConfigImpl() -> ContextConfig& {
    return m_context->config;
}

inline auto OptionGroup::getConfigImpl() const -> const ContextConfig& {
    return m_context->config;
}
} // End namespace Argon

#endif // ARGON_OPTION_GROUP_INCLUDE
