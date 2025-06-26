#ifndef ARGON_OPTION_INCLUDE
#define ARGON_OPTION_INCLUDE

#include <charconv>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

#include "Argon/Flag.hpp"
#include "Argon/Options/OptionCharBase.hpp"
#include "Argon/Options/OptionComponent.hpp"
#include "Argon/Options/SetValue.hpp"
#include "Argon/ParserConfig.hpp"
#include "Argon/Traits.hpp"

namespace Argon {
    class IOption;
    template <typename T, typename Enable> class Option;
    class OptionGroup;
    class Context;
    class Parser;
    class ParserConfig;

    class IsSingleOption {};

    template <typename T, typename Enable = void>
    class Option : public HasFlag<Option<T, Enable>>, public SetSingleValueImpl<Option<T, Enable>, T>,
                   public OptionComponent<Option<T, Enable>>, public IsSingleOption {
        using SetSingleValueImpl<Option, T>::setValue;
    public:
        Option() = default;
        explicit Option(T defaultValue);
        explicit Option(T *out);
        Option(T defaultValue, T *out);
    protected:
        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };

    template <typename T>
    class Option<T, std::enable_if_t<is_numeric_char_type<T>>>
        : public HasFlag<Option<T>>, public SetSingleValueImpl<Option<T>, T>,
          public OptionComponent<Option<T>>, public IsSingleOption, public OptionCharBase<Option<T>> {
        using SetSingleValueImpl<Option, T>::setValue;
    public:
        Option() = default;
        explicit Option(T defaultValue);
        explicit Option(T *out);
        Option(T defaultValue, T *out);
    protected:
        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };

    class OptionGroup : public HasFlag<OptionGroup>, public OptionComponent<OptionGroup>{
        std::unique_ptr<Context> m_context;
    public:
        OptionGroup();
        OptionGroup(const OptionGroup&);
        auto operator=(const OptionGroup&) -> OptionGroup&;

        OptionGroup(OptionGroup&&) = default;
        auto operator=(OptionGroup&&) -> OptionGroup& = default;

        template <typename T> requires DerivesFrom<T, IOption>
        auto operator+(T&& other) & -> OptionGroup&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto operator+(T&& other) && -> OptionGroup;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto getOption(std::string_view flag) -> IOption *;

        auto getContext() -> Context&;

        [[nodiscard]] auto getContext() const -> const Context&;
    };

    class IsPositional {
    protected:
        IsPositional() = default;
    public:
        virtual ~IsPositional() = default;
        [[nodiscard]] virtual auto cloneAsPositional() const -> std::unique_ptr<IsPositional> = 0;
    };

    template <typename T, typename Enable = void>
    class Positional : public SetSingleValueImpl<Positional<T, Enable>, T>,
                       public OptionComponent<Positional<T, Enable>>, public IsPositional {
        using SetSingleValueImpl<Positional, T>::setValue;
    public:
        Positional() = default;

        explicit Positional(T defaultValue);

        explicit Positional(T *out);

        Positional(T defaultValue, T *out);

        [[nodiscard]] auto cloneAsPositional() const -> std::unique_ptr<IsPositional> override;
    protected:
        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };

    template <typename T>
    class Positional<T, std::enable_if_t<is_numeric_char_type<T>>>
        : public SetSingleValueImpl<Positional<T>, T>, public OptionComponent<Positional<T>>,
          public IsPositional, public OptionCharBase<Positional<T>> {
        using SetSingleValueImpl<Positional, T>::setValue;
    public:
        Positional() = default;

        explicit Positional(T defaultValue);

        explicit Positional(T *out);

        Positional(T defaultValue, T *out);

        [[nodiscard]] auto cloneAsPositional() const -> std::unique_ptr<IsPositional> override;
    protected:
        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };
}

//------------------------------------------------------Includes--------------------------------------------------------

#include "Argon/Context.hpp"

//---------------------------------------------------Implementations----------------------------------------------------

#include "Argon/Parser.hpp"

namespace Argon {

template<typename T, typename Enable>
Option<T, Enable>::Option(T defaultValue) : SetSingleValueImpl<Option, T>(defaultValue) {}

template<typename T, typename Enable>
Option<T, Enable>::Option(T *out) : SetSingleValueImpl<Option, T>(out) {}

template<typename T, typename Enable>
Option<T, Enable>::Option(T defaultValue, T *out) : SetSingleValueImpl<Option, T>(defaultValue, out) {}

template<typename T, typename Enable>
void Option<T, Enable>::setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) {
    SetSingleValueImpl<Option, T>::setValue(parserConfig, {}, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

template<typename T>
Option<T, std::enable_if_t<is_numeric_char_type<T>>>::Option(T defaultValue) : SetSingleValueImpl<Option, T>(defaultValue) {}

template<typename T>
Option<T, std::enable_if_t<is_numeric_char_type<T>>>::Option(T *out) : SetSingleValueImpl<Option, T>(out) {}

template<typename T>
Option<T, std::enable_if_t<is_numeric_char_type<T>>>::Option(T defaultValue, T *out)
    : SetSingleValueImpl<Option, T>(defaultValue, out) {}

template<typename T>
void Option<T, std::enable_if_t<is_numeric_char_type<T>>>::setValue(const ParserConfig& parserConfig,
                                                                    std::string_view flag, std::string_view value) {
    SetSingleValueImpl<Option, T>::setValue(parserConfig, { .charMode = this->m_charMode }, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

inline OptionGroup::OptionGroup() {
    m_context = std::make_unique<Context>();
}

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

template<typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) & -> OptionGroup& {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template <typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) && -> OptionGroup {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template<typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::addOption(T&& option) -> void {
    m_context->addOption(std::forward<T>(option));
}

template<typename T, typename Enable>
void Positional<T, Enable>::setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) {
    SetSingleValueImpl<Positional, T>::setValue(parserConfig, {}, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

template<typename T, typename Enable>
Positional<T, Enable>::Positional(T defaultValue) : SetSingleValueImpl<Positional, T>(defaultValue) {}

template<typename T, typename Enable>
Positional<T, Enable>::Positional(T *out) : SetSingleValueImpl<Positional, T>(out) {}

template<typename T, typename Enable>
Positional<T, Enable>::Positional(T defaultValue, T *out) : SetSingleValueImpl<Positional, T>(defaultValue, out) {}

template<typename T, typename Enable>
auto Positional<T, Enable>::cloneAsPositional() const -> std::unique_ptr<IsPositional> {
    return std::make_unique<Positional>(*this);
}

template<typename T>
Positional<T, std::enable_if_t<is_numeric_char_type<T>>>::Positional(T defaultValue)
    : SetSingleValueImpl<Positional, T>(defaultValue) {}

template<typename T>
Positional<T, std::enable_if_t<is_numeric_char_type<T>>>::Positional(T *out)
    : SetSingleValueImpl<Positional, T>(out) {}

template<typename T>
Positional<T, std::enable_if_t<is_numeric_char_type<T>>>::Positional(T defaultValue, T *out)
    : SetSingleValueImpl<Positional, T>(defaultValue, out) {}

template<typename T>
auto Positional<T, std::enable_if_t<is_numeric_char_type<T>>>::cloneAsPositional() const -> std::unique_ptr<IsPositional> {
    return std::make_unique<Positional>(*this);
}

template<typename T>
auto Positional<T, std::enable_if_t<is_numeric_char_type<T>>>::setValue(const ParserConfig& parserConfig,
    std::string_view flag, std::string_view value) -> void {
    SetSingleValueImpl<Positional, T>::setValue(parserConfig, { .charMode = this->m_charMode }, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

inline auto OptionGroup::getOption(std::string_view flag) -> IOption* { //NOLINT (function is not const)
    return m_context->getFlagOption(flag);
}

inline auto OptionGroup::getContext() -> Context& { //NOLINT (function is not const)
    return *m_context;
}

inline auto OptionGroup::getContext() const -> const Context & {
    return *m_context;
}
} // End namespace Argon

#endif // ARGON_OPTION_INCLUDE