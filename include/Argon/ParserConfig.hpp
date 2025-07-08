#ifndef ARGON_PARSERCONFIG_INCLUDE
#define ARGON_PARSERCONFIG_INCLUDE

#include <cassert>
#include <functional>
#include <string_view>
#include <typeindex>

#include <Argon/Traits.hpp>

namespace Argon {

using DefaultConversionFn = std::function<bool(std::string_view, void*)>;
using DefaultConversions  = std::unordered_map<std::type_index, DefaultConversionFn>;

enum class CharMode {
    UseDefault = 0,
    ExpectAscii,
    ExpectInteger,
};

enum class PositionalPolicy {
    UseDefault = 0,
    Interleaved,
    BeforeFlags,
    AfterFlags,
};

struct IntegralBoundsBase {
    IntegralBoundsBase() = default;
    virtual ~IntegralBoundsBase() = default;
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<IntegralBoundsBase> = 0;
};

template <typename T> requires is_non_bool_number<T>
struct IntegralBounds : IntegralBoundsBase{
    T min = std::numeric_limits<T>::lowest();
    T max = std::numeric_limits<T>::max();

    [[nodiscard]] auto clone() const -> std::unique_ptr<IntegralBoundsBase> override;
};

class BoundsMap {
public:
    std::unordered_map<std::type_index, std::unique_ptr<IntegralBoundsBase>> map;

    BoundsMap() = default;
    BoundsMap(const BoundsMap&);
    BoundsMap& operator=(const BoundsMap&);
    BoundsMap(BoundsMap&&) noexcept = default;
    BoundsMap& operator=(BoundsMap&&) noexcept = default;
};

class ParserConfig {
    DefaultConversions m_defaultConversions;
    CharMode m_defaultCharMode;
    PositionalPolicy m_positionalPolicy;
    BoundsMap m_bounds;
public:
    ParserConfig();

    [[nodiscard]] auto getDefaultCharMode() const -> CharMode;
    auto setDefaultCharMode(CharMode newCharMode) -> ParserConfig&;

    [[nodiscard]] auto getDefaultPositionalPolicy() const -> PositionalPolicy;
    auto setDefaultPositionalPolicy(PositionalPolicy newPolicy) -> ParserConfig&;

    template <typename T>
    auto registerConversionFn(std::function<bool(std::string_view, T*)>conversionFn) -> ParserConfig&;
    [[nodiscard]] auto getDefaultConversions() const -> const DefaultConversions&;

    template <typename T> requires is_non_bool_number<T>
    auto getMin() const -> T;

    template <typename T> requires is_non_bool_number<T>
    auto setMin(T min) -> ParserConfig&;

    template <typename T> requires is_non_bool_number<T>
    auto getMax() const -> T;

    template <typename T> requires is_non_bool_number<T>
    auto setMax(T max) -> ParserConfig&;
};

struct OptionConfigChar {
    CharMode charMode = CharMode::UseDefault;
};

template <typename T>
struct OptionConfigIntegral {
    T min;
    T max;
};

struct IOptionConfig {};

template <typename T>
struct OptionConfig
    : IOptionConfig,
      std::conditional_t<is_numeric_char_type<T>, OptionConfigChar, EmptyBase<0>>,
      std::conditional_t<is_non_bool_number<T>, OptionConfigIntegral<T>, EmptyBase<1>> {
    const DefaultConversionFn *conversionFn = nullptr;
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

inline BoundsMap::BoundsMap(const BoundsMap& other) {
    for (const auto& [typeIndex, bound] : other.map) {
        map[typeIndex] = bound->clone();
    }
}

inline BoundsMap& BoundsMap::operator=(const BoundsMap& other) {
    if (this != &other) {
        for (const auto& [typeIndex, bound] : other.map) {
            map[typeIndex] = bound->clone();
        }
    }
    return *this;
}

inline ParserConfig::ParserConfig() {
    m_defaultCharMode = CharMode::ExpectAscii;
    m_positionalPolicy = PositionalPolicy::Interleaved;
}

inline auto ParserConfig::getDefaultCharMode() const -> CharMode {
    return m_defaultCharMode;
}

inline auto ParserConfig::setDefaultCharMode(const CharMode newCharMode) -> ParserConfig& {
    if (newCharMode == CharMode::UseDefault) {
        throw std::invalid_argument("Default char mode cannot be UseDefault");
    }
    m_defaultCharMode = newCharMode;
    return *this;
}

inline auto ParserConfig::getDefaultPositionalPolicy() const -> PositionalPolicy {
    return m_positionalPolicy;
}

inline auto ParserConfig::setDefaultPositionalPolicy(const PositionalPolicy newPolicy) -> ParserConfig& {
    if (newPolicy == PositionalPolicy::UseDefault) {
        throw std::invalid_argument("Default positional policy cannot be UseDefault");
    }
    m_positionalPolicy = newPolicy;
    return *this;
}

template<typename T> requires is_non_bool_number<T>
auto IntegralBounds<T>::clone() const -> std::unique_ptr<IntegralBoundsBase> {
    return std::make_unique<IntegralBounds>(*this);
}

template<typename T>
auto ParserConfig::registerConversionFn(std::function<bool(std::string_view, T *)> conversionFn) -> ParserConfig& {
    auto wrapper = [conversionFn](std::string_view arg, void *out) -> bool {
        return conversionFn(arg, static_cast<T*>(out));
    };
    m_defaultConversions[std::type_index(typeid(T))] = wrapper;
    return *this;
}

inline auto ParserConfig::getDefaultConversions() const -> const DefaultConversions& {
    return m_defaultConversions;
}

template<typename T> requires is_non_bool_number<T>
auto ParserConfig::getMin() const -> T {
    if (const auto it = m_bounds.map.find(std::type_index(typeid(T))); it != m_bounds.map.end()) {
        return static_cast<IntegralBounds<T>*>(it->second.get())->min;
    }
    return std::numeric_limits<T>::lowest();
}

template<typename T> requires is_non_bool_number<T>
auto ParserConfig::setMin(T min) -> ParserConfig& {
    const auto id = std::type_index(typeid(T));
    if (!m_bounds.map.contains(id)) {
        m_bounds.map[id] = std::make_unique<IntegralBounds<T>>();
    }
    static_cast<IntegralBounds<T> *>(m_bounds.map.at(id).get()).min = min;
    return *this;
}

template<typename T> requires is_non_bool_number<T>
auto ParserConfig::getMax() const -> T {
    if (const auto it = m_bounds.map.find(std::type_index(typeid(T))); it != m_bounds.map.end()) {
        return static_cast<IntegralBounds<T>*>(it->second.get())->max;
    }
    return std::numeric_limits<T>::max();
}

template<typename T> requires is_non_bool_number<T>
auto ParserConfig::setMax(T max) -> ParserConfig& {
    const auto id = std::type_index(typeid(T));
    if (!m_bounds.map.contains(id)) {
        m_bounds.map[id] = std::make_unique<IntegralBounds<T>>();
    }
    static_cast<IntegralBounds<T> *>(m_bounds.map.at(id).get()).max = max;
    return *this;
}

} // End namespace Argon

#endif //ARGON_PARSERCONFIG_INCLUDE