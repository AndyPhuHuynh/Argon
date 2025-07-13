#ifndef ARGON_OPTION_CONFIG_INCLUDE
#define ARGON_OPTION_CONFIG_INCLUDE

#include "Argon/Config/Types.hpp"
#include "Argon/Traits.hpp"

namespace Argon::detail {

struct OptionConfigChar {
    CharMode charMode = CharMode::UseDefault;
};

template <typename T>
struct OptionConfigIntegral {
    T min;
    T max;
};

} // End namespace Argon::detail

namespace Argon {

struct IOptionConfig {};

template <typename T>
struct OptionConfig
    : IOptionConfig,
      std::conditional_t<detail::is_numeric_char_type<T>, detail::OptionConfigChar, detail::EmptyBase<0>>,
      std::conditional_t<detail::is_non_bool_number<T>, detail::OptionConfigIntegral<T>, detail::EmptyBase<1>> {
    const DefaultConversionFn *conversionFn = nullptr;
};

} // End namespace Argon

#endif // ARGON_OPTION_CONFIG_INCLUDE
