#ifndef ARGON_GET_CONFIG_INCLUDE
#define ARGON_GET_CONFIG_INCLUDE

#include "Argon/Config/OptionConfig.hpp"
#include "Argon/Options/IOption.hpp"
#include "Argon/Options/OptionCharBase.hpp"
#include "Argon/Options/OptionIntegralBaseImpl.hpp"
#include "ContextConfig.hpp"

namespace Argon::detail {

template <typename OptionType, typename ValueType>
auto getOptionConfig(const ContextConfig& parserConfig, const IOption *option) -> OptionConfig<ValueType> {
    OptionConfig<ValueType> optionConfig;
    if constexpr (is_numeric_char_type<ValueType>) {
        if (const auto charOpt = dynamic_cast<const OptionCharBase<OptionType>*>(option); charOpt != nullptr) {
            optionConfig.charMode = resolveCharMode(parserConfig.getDefaultCharMode(), charOpt->getCharMode());
        }
    }
    if constexpr (is_non_bool_number<ValueType>) {
        if (const auto numOpt = dynamic_cast<const OptionIntegralBase<ValueType>*>(option); numOpt != nullptr) {
            optionConfig.min = numOpt->getMin().has_value() ? numOpt->getMin().value() : parserConfig.getMin<ValueType>();
            optionConfig.max = numOpt->getMax().has_value() ? numOpt->getMax().value() : parserConfig.getMax<ValueType>();
        }
    }
    if (const auto it = parserConfig.getDefaultConversions().find(std::type_index(typeid(ValueType)));
        it != parserConfig.getDefaultConversions().end()) {
        optionConfig.conversionFn = &it->second;
    }
    return optionConfig;
}

} // End namespace Argon::detail
#endif // ARGON_GET_CONFIG_INCLUDE
