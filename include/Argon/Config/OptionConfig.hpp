#ifndef ARGON_OPTION_CONFIG_INCLUDE
#define ARGON_OPTION_CONFIG_INCLUDE

#include "Argon/Config/Types.hpp"
#include "Argon/Traits.hpp"

namespace Argon {

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

} // End namespace Argon::detail

#endif // ARGON_OPTION_CONFIG_INCLUDE
