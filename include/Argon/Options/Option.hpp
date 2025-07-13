#ifndef ARGON_OPTION_INCLUDE
#define ARGON_OPTION_INCLUDE

#include "../Config/ConfigHelpers.hpp"
#include "OptionTypeExtensions.hpp"
#include "Argon/Flag.hpp"
#include "Argon/Options/OptionComponent.hpp"
#include "Argon/Options/SetValue.hpp"
#include "../Config/ContextConfig.hpp"

namespace Argon {
class IsSingleOption {};

template <typename T>
class Option
    : public IsSingleOption,
      public HasFlag<Option<T>>,
      public SetSingleValueImpl<Option<T>, T>,
      public OptionComponent<Option<T>>,
      public detail::OptionTypeExtensions<Option<T>, T> {
    using SetSingleValueImpl<Option, T>::setValue;
public:
    Option() = default;

    explicit Option(T defaultValue) : SetSingleValueImpl<Option, T>(defaultValue) {}

    explicit Option(T *out) : SetSingleValueImpl<Option, T>(out) {};

    Option(T defaultValue, T *out) : SetSingleValueImpl<Option, T>(defaultValue, out) {};
protected:
    auto setValue(const ContextConfig& parserConfig, std::string_view flag, std::string_view value) -> void override {
        auto optionConfig = detail::getOptionConfig<Option, T>(parserConfig, this);
        SetSingleValueImpl<Option, T>::setValue(optionConfig, flag, value);
        this->m_error = this->getConversionError();
        this->m_isSet = true;
    }
};

} // End namespace Argon

#endif // ARGON_OPTION_INCLUDE