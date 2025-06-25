#ifndef ARGON_PARSERCONFIG_INCLUDE
#define ARGON_PARSERCONFIG_INCLUDE

#include <functional>
#include <string_view>
#include <typeindex>

namespace Argon {
    using DefaultConversionFn = std::function<bool(std::string_view, void*)>;
    using DefaultConversions  = std::unordered_map<std::type_index, DefaultConversionFn>;

    enum class CharMode {
        None = 0,
        ExpectAscii,
        ExpectInteger,
    };

    enum class PositionalPolicy {
        None = 0,
        Interleaved,
        BeforeFlags,
        AfterFlags,
    };

    class ParserConfig {
        DefaultConversions m_defaultConversions;
        CharMode m_defaultCharMode;
        PositionalPolicy m_positionalPolicy;

    public:
        ParserConfig();

        [[nodiscard]] auto getCharMode() const -> CharMode;
        auto setCharMode(CharMode newCharMode) -> ParserConfig&;

        [[nodiscard]] auto getPositionalPolicy() const -> PositionalPolicy;
        auto setPositionalPolicy(PositionalPolicy newPolicy) -> ParserConfig&;

        template <typename T>
        auto registerConversionFn(std::function<bool(std::string_view, T*)>conversionFn) -> void;
        [[nodiscard]] auto getDefaultConversions() const -> const DefaultConversions&;
    };

    struct OptionConfig {
        CharMode charMode = CharMode::None;
    };
}

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {
inline ParserConfig::ParserConfig() {
    m_defaultCharMode = CharMode::ExpectAscii;
    m_positionalPolicy = PositionalPolicy::Interleaved;
}

inline auto ParserConfig::getCharMode() const -> CharMode {
    return m_defaultCharMode;
}

inline auto ParserConfig::setCharMode(const CharMode newCharMode) -> ParserConfig& {
    if (newCharMode == CharMode::None) {
        throw std::invalid_argument("Default char mode cannot be none");
    }
    m_defaultCharMode = newCharMode;
    return *this;
}

inline auto ParserConfig::getPositionalPolicy() const -> PositionalPolicy {
    return m_positionalPolicy;
}

inline auto ParserConfig::setPositionalPolicy(const PositionalPolicy newPolicy) -> ParserConfig& {
    if (newPolicy == PositionalPolicy::None) {
        throw std::invalid_argument("Default positional policy cannot be none");
    }
    m_positionalPolicy = newPolicy;
    return *this;
}

template<typename T>
auto ParserConfig::registerConversionFn(std::function<bool(std::string_view, T *)> conversionFn) -> void {
    auto wrapper = [conversionFn](std::string_view arg, void *out) -> bool {
        return conversionFn(arg, static_cast<T*>(out));
    };
    m_defaultConversions[std::type_index(typeid(T))] = wrapper;
}

inline auto ParserConfig::getDefaultConversions() const -> const DefaultConversions& {
    return m_defaultConversions;
}

} // End namespace Argon

#endif //ARGON_PARSERCONFIG_INCLUDE