#ifndef ARGON_OPTION_TYPE_EXTENSIONS_INCLUDE
#define ARGON_OPTION_TYPE_EXTENSIONS_INCLUDE

#include "Argon/Options/OptionCharBase.hpp"
#include "Argon/Options/OptionIntegralBaseImpl.hpp"
#include "Argon/Traits.hpp"

namespace Argon {

template <typename Derived, typename T>
class OptionTypeExtensions
    : public std::conditional_t<is_numeric_char_type<T>, OptionCharBase<Derived>, EmptyBase<0>>,
      public std::conditional_t<is_non_bool_number<T>, OptionIntegralBaseImpl<Derived, T>, EmptyBase<1>> {
};

}

#endif // ARGON_OPTION_TYPE_EXTENSIONS_INCLUDE
