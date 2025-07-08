#ifndef ARGON_POSITIONAL_INCLUDE
#define ARGON_POSITIONAL_INCLUDE

#include "Argon/Options/OptionCharBase.hpp"
#include "Argon/Options/OptionComponent.hpp"
#include "Argon/Options/SetValue.hpp"

namespace Argon {
class IsPositional {
protected:
    IsPositional() = default;
public:
    virtual ~IsPositional() = default;
};

template <typename T>
class Positional
    : public IsPositional,
      public SetSingleValueImpl<Positional<T>, T>,
      public OptionComponent<Positional<T>>,
      public OptionTypeExtensions<Positional<T>, T> {
    using SetSingleValueImpl<Positional, T>::setValue;
public:
    Positional() = default;

    explicit Positional(T defaultValue) : SetSingleValueImpl<Positional, T>(defaultValue) {}

    explicit Positional(T *out) : SetSingleValueImpl<Positional, T>(out) {}

    Positional(T defaultValue, T *out) : SetSingleValueImpl<Positional, T>(defaultValue, out) {}

protected:
    auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override {
        OptionConfig<T> optionConfig = detail::getOptionConfig<Positional, T>(parserConfig, this);
        SetSingleValueImpl<Positional, T>::setValue(optionConfig, flag, value);
        this->m_error = this->getConversionError();
        this->m_isSet = true;
    }
};
}
#endif // ARGON_POSITIONAL_INCLUDE
