#ifndef ARGON_SET_VALUE_INCLUDE
#define ARGON_SET_VALUE_INCLUDE

#include <functional>
#include <sstream>
#include <string>
#include <string_view>

#include "Argon/Config/OptionConfig.hpp"
#include "Argon/Config/ContextConfig.hpp"
#include "Argon/StringUtil.hpp"
#include "Argon/Traits.hpp"

namespace Argon {

enum class Base {
    Invalid = 0,
    Binary = 2,
    Decimal = 10,
    Hexadecimal = 16,
};

inline auto getBaseFromPrefix(const std::string_view arg) -> Base {
    size_t zeroIndex = 0, baseIndex = 1;
    if (!arg.empty() && (arg[0] == '-' || arg[0] == '+')) {
        zeroIndex = 1;
        baseIndex = 2;
    }

    if (arg.length() <= baseIndex)
        return Base::Decimal;

    if (arg[zeroIndex] != '0' || std::isdigit(arg[baseIndex]))  return Base::Decimal;
    if (arg[baseIndex] == 'b' || arg[baseIndex] == 'B')         return Base::Binary;
    if (arg[baseIndex] == 'x' || arg[baseIndex] == 'X')         return Base::Hexadecimal;

    return Base::Invalid;
}

template <typename T> requires std::is_integral_v<T>
auto parseIntegralType(const OptionConfig<T>& config, const std::string_view arg, T& out) -> bool {
    if (arg.empty()) return false;
    const auto base = getBaseFromPrefix(arg);
    if (base == Base::Invalid) return false;

    // Calculate begin offset
    const bool hasSignPrefix = arg[0] == '-' || arg[0] == '+';
    size_t beginOffset = 0;
    if (base != Base::Decimal)  { beginOffset += 2; }
    if (hasSignPrefix)          { beginOffset += 1; }

    const std::string_view digits = arg.substr(beginOffset);
    if (digits.empty()) return false;

    std::string noBasePrefix;
    if (hasSignPrefix) {
        noBasePrefix += arg[0];
    }
    noBasePrefix += digits;

    // Calculate begin and end pointers
    const char *begin = noBasePrefix.data();
    const char *end   = noBasePrefix.data() + noBasePrefix.size();
    if (begin == end) return false;

    auto [ptr, ec] = std::from_chars(begin, end, out, static_cast<int>(base));
    if (out < config.min || out > config.max) return false;
    return ec == std::errc() && ptr == end;
}

inline auto parseBool(const std::string_view arg, bool& out) -> bool {
    using namespace detail;
    if (iequals(arg, "true") || iequals(arg, "1") || iequals(arg, "yes")    || iequals(arg, "on") ||
        iequals(arg, "y")    || iequals(arg, "t") || iequals(arg, "enable") || iequals(arg, "enabled")) {
        out = true;
        return true;
    }

    if (iequals(arg, "false") || iequals(arg, "0") || iequals(arg, "no")      || iequals(arg, "off") ||
        iequals(arg, "n")     || iequals(arg, "f") || iequals(arg, "disable") || iequals(arg, "disabled")) {
        out = false;
        return true;
    }
    return false;
}

template <typename T> requires detail::is_numeric_char_type<T>
auto parseNumericChar(const OptionConfig<T>& config, const std::string_view arg, T& out) -> bool {
    if (config.charMode == CharMode::ExpectInteger) {
        return parseIntegralType(config, arg, out);
    }
    if (arg.length() == 1) {
        out = static_cast<T>(arg[0]);
        return true;
    }
    if (arg.length() == 3) {
        if ((arg[0] == '\'' && arg[2] == '\'') || (arg[0] == '\"' && arg[2] == '\"')) {
            out = static_cast<T>(arg[1]);
            return true;
        }
    }
    return false;
}

template <typename T> requires std::is_floating_point_v<T>
auto parseFloatingPoint(const OptionConfig<T>& config, const std::string_view arg, T& out) -> bool {
    if (arg.empty()) return false;

    const char *cstr = arg.data();
    char *end = nullptr;
    errno = 0;

    if constexpr (std::is_same_v<T, float>) {
        out = std::strtof(cstr, &end);
    } else if constexpr (std::is_same_v<T, double>) {
        out = std::strtod(cstr, &end);
    } else if constexpr (std::is_same_v<T, long double>) {
        out = std::strtold(cstr, &end);
    }

    if (out < config.min || out > config.max) return false;
    return errno == 0 && end == cstr + arg.length();
}

class ISetValue {
public:
    ISetValue() = default;
    virtual ~ISetValue() = default;

    virtual void setValue(const ContextConfig& parserConfig, std::string_view flag, std::string_view value) = 0;
    virtual void setValue(const IOptionConfig& optionConfig, std::string_view flag, std::string_view value) = 0;
};

template <typename T>
using ConversionFn = std::function<bool(std::string_view, T&)>;
using GenerateErrorMsgFn = std::function<std::string(std::string_view, std::string_view)>;

template <typename Derived, typename T>
class Converter {
protected:
    ConversionFn<T> m_conversion_fn = nullptr;
    GenerateErrorMsgFn m_generate_error_msg_fn = nullptr;
    std::string m_conversionError;

    auto generateErrorMsg(const OptionConfig<T>& config, std::string_view optionName, std::string_view invalidArg) -> void {
        // Generate custom error message if provided
        if (this->m_generate_error_msg_fn != nullptr) {
            this->m_conversionError = this->m_generate_error_msg_fn(optionName, invalidArg);
            return;
        }

        // Else generate default message
        std::stringstream ss;

        if (optionName.empty()) {
            ss << "Invalid value: ";
        } else {
            ss << std::format(R"(Invalid value for "{}": )", optionName);
        }

        if constexpr (detail::is_numeric_char_type<T>) {
            if (config.charMode == CharMode::ExpectAscii) {
                ss << "expected ASCII character";
            } else {
                ss << std::format(
                    "expected integer between {} and {} inclusive",
                    detail::format_with_commas(config.min),
                    detail::format_with_commas(config.max));
            }
        } else if constexpr (detail::is_non_bool_integral<T>) {
            ss << std::format(
                "expected integer between {} and {} inclusive",
                detail::format_with_commas(config.min),
                detail::format_with_commas(config.max));
        } else if constexpr (std::is_same_v<T, bool>) {
            ss << "expected boolean";
        } else if constexpr (std::is_floating_point_v<T>) {
            if (config.min != std::numeric_limits<T>::lowest() && config.max != std::numeric_limits<T>::max()) {
                ss << std::format("expected floating point number between {} and {} inclusive",
                    detail::format_with_commas(config.min),
                    detail::format_with_commas(config.max));
            } else if (config.max != std::numeric_limits<T>::max()) {
                ss << std::format("expected floating point number less than or equal to {}",
                    detail::format_with_commas(config.max));
            } else if (config.min != std::numeric_limits<T>::lowest()) {
                ss << std::format("expected floating point number greater than or equal to {}",
                    detail::format_with_commas(config.min));
            } else {
                ss << "expected floating point number";
            }
        } else {
            ss << std::format("expected {}", detail::getTypeName<T>());
        }

        // Actual value
        ss << std::format(R"(, got: "{}")", invalidArg);
        this->m_conversionError = ss.str();
    }

public:
    auto convert(
         const OptionConfig<T>& config, const std::string_view flag, std::string_view value, T& outValue
    ) -> void {
        m_conversionError.clear();
        bool success;
        // Use custom conversion function for this specific option if supplied
        if (this->m_conversion_fn != nullptr) {
            success = this->m_conversion_fn(value, outValue);
        }
        // Search for conversion list for conversion for this type if specified
        else if (config.conversionFn != nullptr) {
            success = (*config.conversionFn)(value, static_cast<void*>(&outValue));
        }
        // Fallback to generic parsing
        // Parse as a character
        else if constexpr (detail::is_numeric_char_type<T>) {
            success = parseNumericChar<T>(config, value, outValue);
        }
        // Parse as a floating point
        else if constexpr (std::is_floating_point_v<T>) {
            success = parseFloatingPoint<T>(config, value, outValue);
        }
        // Parse as non-bool integral if valid
        else if constexpr (detail::is_non_bool_integral<T>) {
            success = parseIntegralType<T>(config, value, outValue);
        }
        // Parse as boolean if T is a boolean
        else if constexpr (std::is_same_v<T, bool>) {
            success = parseBool(value, outValue);
        }
        // Parse as a string
        else if constexpr (std::is_same_v<T, std::string>) {
            outValue = value;
            success = true;
        }
        // Use stream extraction if custom conversion not supplied and type is not integral
        else if constexpr (detail::has_stream_extraction<T>::value) {
            auto iss = std::istringstream(std::string(value));
            iss >> outValue;
            success = !iss.fail() && iss.eof();
        }
        // Should never reach this
        else {
            throw std::runtime_error("Type does not support stream extraction, "
                                     "was not an integral type, and no converter was provided.");
        }
        // Set error if not successful
        if (!success) {
            generateErrorMsg(config, flag, value);
        }
    }

    [[nodiscard]] auto hasConversionError() const -> bool {
        return !m_conversionError.empty();
    }

    auto getConversionError() -> std::string {
        return m_conversionError;
    }

    auto withConversionFn(const ConversionFn<T>& conversion_fn) & -> Derived& {
        m_conversion_fn = conversion_fn;
        return static_cast<Derived&>(*this);
    }

    auto withConversionFn(const ConversionFn<T>& conversion_fn) && -> Derived&& {
        m_conversion_fn = conversion_fn;
        return static_cast<Derived&&>(*this);
    }

    auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) & -> Derived& {
        m_generate_error_msg_fn = generate_error_msg_fn;
        return static_cast<Derived&>(*this);
    }

    auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) && -> Derived&& {
        m_generate_error_msg_fn = generate_error_msg_fn;
        return static_cast<Derived&&>(*this);
    }
};

template <typename Derived, typename T>
class SetSingleValueImpl : public ISetValue, public Converter<Derived, T> {
    T m_value = T();
    T *m_out = nullptr;
public:
    SetSingleValueImpl() = default;

    explicit SetSingleValueImpl(T defaultValue) : m_value(defaultValue) {}

    explicit SetSingleValueImpl(T *out) : m_out(out) {}

    SetSingleValueImpl(T defaultValue, T *out) : m_value(defaultValue), m_out(out) {
        if (m_out != nullptr) *m_out = m_value;
    }

    [[nodiscard]] auto getValue() const -> const T& {
        return m_value;
    }
protected:
    auto setValue(const IOptionConfig& optionConfig, std::string_view flag, std::string_view value) -> void override {
        T temp;
        this->convert(static_cast<const OptionConfig<T>&>(optionConfig), flag, value, temp);
        if (this->hasConversionError()) {
            return;
        }
        m_value = temp;
        if (m_out != nullptr) {
            *m_out = temp;
        }
    }
};

} // End namespace Argon

#endif // ARGON_SET_VALUE_INCLUDE