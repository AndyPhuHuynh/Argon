#ifndef ARGON_PARSERCONFIG_INCLUDE
#define ARGON_PARSERCONFIG_INCLUDE

#include <cassert>
#include <functional>
#include <string_view>
#include <typeindex>

#include "Argon/Config/Types.hpp"
#include "Argon/Traits.hpp"

namespace Argon {

class ContextConfig {
    DefaultConversions m_defaultConversions;
    CharMode m_defaultCharMode = CharMode::ExpectAscii;
    PositionalPolicy m_positionalPolicy = PositionalPolicy::Interleaved;
    detail::BoundsMap m_bounds;
public:
    [[nodiscard]] auto getDefaultCharMode() const -> CharMode;
    auto setDefaultCharMode(CharMode newCharMode) -> ContextConfig&;

    [[nodiscard]] auto getDefaultPositionalPolicy() const -> PositionalPolicy;
    auto setDefaultPositionalPolicy(PositionalPolicy newPolicy) -> ContextConfig&;

    template <typename T>
    auto registerConversionFn(const std::function<bool(std::string_view, T *)>& conversionFn) -> ContextConfig&;
    [[nodiscard]] auto getDefaultConversions() const -> const DefaultConversions&;

    template <typename T> requires is_non_bool_number<T>
    auto getMin() const -> T;

    template <typename T> requires is_non_bool_number<T>
    auto setMin(T min) -> ContextConfig&;

    template <typename T> requires is_non_bool_number<T>
    auto getMax() const -> T;

    template <typename T> requires is_non_bool_number<T>
    auto setMax(T max) -> ContextConfig&;
};

} // End namespace Argon

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon::detail {

inline auto resolveCharMode(
    const CharMode defaultMode, const CharMode otherMode
) -> CharMode {
    assert(defaultMode != CharMode::UseDefault&& "Default char mode must be not use default");
    return otherMode == CharMode::UseDefault ? defaultMode : otherMode;
}

inline auto resolvePositionalPolicy(
    const PositionalPolicy defaultPolicy, const PositionalPolicy contextPolicy
) -> PositionalPolicy {
    assert(defaultPolicy != PositionalPolicy::UseDefault && "Default positional policy must be not use default");
    return contextPolicy == PositionalPolicy::UseDefault ? defaultPolicy : contextPolicy;
}

} // End namespace Argon::detail

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {

inline auto ContextConfig::getDefaultCharMode() const -> CharMode {
    return m_defaultCharMode;
}

inline auto ContextConfig::setDefaultCharMode(const CharMode newCharMode) -> ContextConfig& {
    if (newCharMode == CharMode::UseDefault) {
        throw std::invalid_argument("Default char mode cannot be UseDefault");
    }
    m_defaultCharMode = newCharMode;
    return *this;
}

inline auto ContextConfig::getDefaultPositionalPolicy() const -> PositionalPolicy {
    return m_positionalPolicy;
}

inline auto ContextConfig::setDefaultPositionalPolicy(const PositionalPolicy newPolicy) -> ContextConfig& {
    if (newPolicy == PositionalPolicy::UseDefault) {
        throw std::invalid_argument("Default positional policy cannot be UseDefault");
    }
    m_positionalPolicy = newPolicy;
    return *this;
}

template<typename T>
auto ContextConfig::registerConversionFn(const std::function<bool(std::string_view, T *)>& conversionFn) -> ContextConfig& {
    auto wrapper = [conversionFn](std::string_view arg, void *out) -> bool {
        return conversionFn(arg, static_cast<T*>(out));
    };
    m_defaultConversions[std::type_index(typeid(T))] = wrapper;
    return *this;
}

inline auto ContextConfig::getDefaultConversions() const -> const DefaultConversions& {
    return m_defaultConversions;
}

template<typename T> requires is_non_bool_number<T>
auto ContextConfig::getMin() const -> T {
    if (const auto it = m_bounds.map.find(std::type_index(typeid(T))); it != m_bounds.map.end()) {
        return static_cast<detail::IntegralBounds<T>*>(it->second.get())->min;
    }
    return std::numeric_limits<T>::lowest();
}

template<typename T> requires is_non_bool_number<T>
auto ContextConfig::setMin(T min) -> ContextConfig& {
    const auto id = std::type_index(typeid(T));
    if (!m_bounds.map.contains(id)) {
        m_bounds.map[id] = std::make_unique<detail::IntegralBounds<T>>();
    }
    static_cast<detail::IntegralBounds<T> *>(m_bounds.map.at(id).get()).min = min;
    return *this;
}

template<typename T> requires is_non_bool_number<T>
auto ContextConfig::getMax() const -> T {
    if (const auto it = m_bounds.map.find(std::type_index(typeid(T))); it != m_bounds.map.end()) {
        return static_cast<detail::IntegralBounds<T>*>(it->second.get())->max;
    }
    return std::numeric_limits<T>::max();
}

template<typename T> requires is_non_bool_number<T>
auto ContextConfig::setMax(T max) -> ContextConfig& {
    const auto id = std::type_index(typeid(T));
    if (!m_bounds.map.contains(id)) {
        m_bounds.map[id] = std::make_unique<detail::IntegralBounds<T>>();
    }
    static_cast<detail::IntegralBounds<T> *>(m_bounds.map.at(id).get()).max = max;
    return *this;
}

} // End namespace Argon

#endif //ARGON_PARSERCONFIG_INCLUDE