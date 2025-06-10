#ifndef ARGON_OPTION_INCLUDE
#define ARGON_OPTION_INCLUDE

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Flag.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename T> class Option;
    class OptionGroup;
    class Context;

    class IOption {
    protected:
        Flag m_flag;
        std::string m_error;
        std::string m_description;

        bool m_isSet = false;
        IOption() = default;
    public:
        IOption(const IOption&);
        IOption& operator=(const IOption&);
        IOption(IOption&&) noexcept;
        IOption& operator=(IOption&&) noexcept;
        virtual ~IOption() = default;

        auto applyPrefixes(std::string_view shortPrefix, std::string_view longPrefix) -> void;

        [[nodiscard]] auto getFlag() const -> const Flag&;

        [[nodiscard]] auto getError() const -> const std::string&;

        [[nodiscard]] auto hasError() const -> bool;

        [[nodiscard]] auto getDescription() const -> const std::string&;

        auto clearError() -> void;

        [[nodiscard]] auto isSet() const -> bool;

        [[nodiscard]] virtual auto clone() const -> std::unique_ptr<IOption> = 0;
    };

    template <typename Derived>
    class OptionComponent : public IOption {
        friend Derived;
        bool m_mainFlagSet = false;

        OptionComponent() = default;
    public:
        [[nodiscard]] std::unique_ptr<IOption> clone() const override;

        auto operator[](std::string_view tag) & -> Derived&;
        auto operator[](std::string_view tag) && -> Derived&&;

        auto description(std::string_view desc) & -> Derived&;
        auto description(std::string_view desc) && -> Derived&&;
    };

    class OptionBase {
    public:
        OptionBase() = default;
        virtual ~OptionBase() = default;

        virtual void setValue(const std::string& flag, const std::string& value) = 0;
    };

    template <typename T>
    using ConversionFn = std::function<bool(const std::string&, T&)>;
    using GenerateErrorMsgFn = std::function<std::string(const std::string&, const std::string&)>;

    template <typename Derived, typename T>
    class Converter {
    protected:
        ConversionFn<T> m_conversion_fn = nullptr;
        GenerateErrorMsgFn m_generate_error_msg_fn = nullptr;
        std::string m_conversionError;

        auto generateErrorMsg(const std::string& flag, const std::string& invalidArg) -> void;

    public:
        auto convert(const std::string& flag, const std::string& value, T& outValue) -> void;

        [[nodiscard]] auto hasConversionError() const -> bool;

        auto getConversionError() -> std::string;

        auto withConversionFn(const ConversionFn<T>& conversion_fn) & -> Derived&;

        auto withConversionFn(const ConversionFn<T>& conversion_fn) && -> Derived&&;

        auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) & -> Derived&;

        auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) && -> Derived&&;
    };

    class IsSingleOption {};

    template <typename T>
    class Option : public OptionBase, public OptionComponent<Option<T>>,
                   public IsSingleOption, public Converter<Option<T>, T> {
        T m_value = T();
        T *m_out = nullptr;
    public:
        Option() = default;

        explicit Option(T defaultValue);

        explicit Option(T *out);

        Option(T defaultValue, T *out);

        auto getValue() const -> const T&;
    private:
        auto setValue(const std::string& flag, const std::string& value) -> void override;
    };

    class OptionGroup : public OptionComponent<OptionGroup> {
        std::unique_ptr<Context> m_context;
    public:
        OptionGroup();
        OptionGroup(const OptionGroup&);
        auto operator=(const OptionGroup&) -> OptionGroup&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto operator+(T&& other) & -> OptionGroup&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto operator+(T&& other) && -> OptionGroup&&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto getOption(const std::string& flag) -> IOption*;

        auto getContext() -> Context&;

        [[nodiscard]] auto getContext() const -> const Context&;
    };
}

//------------------------------------------------------Includes--------------------------------------------------------

#include "Context.hpp" //NOLINT (unused include)
#include "StringUtil.hpp"

//---------------------------------------------------Free Functions-----------------------------------------------------

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
auto parseIntegralType(const std::string& arg, T& out) -> bool {
    if (arg.empty()) return false;
    const auto base = getBaseFromPrefix(arg);
    if (base == Base::Invalid) return false;

    // Calculate begin offset
    int beginOffset = 0;
    if (base != Base::Decimal)          { beginOffset += 2; }
    if (arg[0] == '-' || arg[0] == '+') { beginOffset += 1; }

    // Calculate begin and end pointers
    const char *begin = arg.data() + beginOffset;
    const char *end   = arg.data() + arg.size();
    if (begin == end) return false;

    auto [ptr, ec] = std::from_chars(begin, end, out, static_cast<int>(base));

    // Check for negative sign
    if (arg[0] == '-') {
        if constexpr (std::is_unsigned_v<T>) {
            return false;
        } else {
            out *= -1;
        }
    }
    return ec == std::errc() && ptr == end;
}

inline auto parseBool(const std::string& arg, bool& out) -> bool {
    std::string boolStr = arg;
    to_lower(boolStr);
    if (boolStr == "true") {
        out = true;
        return true;
    }
    if (boolStr == "false") {
        out = false;
        return true;
    }
    return false;
}

template <typename T> requires is_numeric_char_type<T>
auto parseNumericChar(const std::string& arg, T& out) -> bool {
    if (arg.length() == 1) {
        out = static_cast<T>(arg[0]);
        return true;
    }
    return parseIntegralType(arg, out);
}

template <typename T> requires std::is_floating_point_v<T>
auto parseFloatingPoint(const std::string& arg, T& out) -> bool {
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

    return errno == 0 && end == cstr + arg.length();
}

} // Namespace Argon

//---------------------------------------------------Implementations----------------------------------------------------

namespace Argon {
template <typename Derived>
auto OptionComponent<Derived>::clone() const -> std::unique_ptr<IOption> {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
}

template <typename Derived>
auto OptionComponent<Derived>::operator[](const std::string_view tag) & -> Derived& {
    if (!m_mainFlagSet) {
        m_mainFlagSet = true;
        m_flag.mainFlag = tag;
    } else {
        m_flag.aliases.emplace_back(tag);
    }
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto OptionComponent<Derived>::operator[](const std::string_view tag) && -> Derived&& {
    if (!m_mainFlagSet) {
        m_mainFlagSet = true;
        m_flag.mainFlag = tag;
    } else {
        m_flag.aliases.emplace_back(tag);
    }
    return static_cast<Derived&&>(*this);
}

template<typename Derived>
auto OptionComponent<Derived>::description(const std::string_view desc) & -> Derived& {
    m_description = desc;
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto OptionComponent<Derived>::description(const std::string_view desc) && -> Derived&& {
    m_description = desc;
    return static_cast<Derived&&>(*this);
}

template <typename Derived, typename T>
auto Converter<Derived, T>::generateErrorMsg(const std::string& flag, const std::string& invalidArg) -> void {
    // Generate custom error message if provided
    if (this->m_generate_error_msg_fn != nullptr) {
        this->m_conversionError = this->m_generate_error_msg_fn(flag, invalidArg);
        return;
    }

    // Else generate default message
    std::stringstream ss;
    ss << "Invalid value for flag \'" << flag << "\': "
        << "expected " << type_name<T>();

    if constexpr (is_non_bool_integral<T>) {
        ss << " in the range of [" << format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::min())) <<
              " to " << format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::max())) << "]";
    } else if constexpr (std::is_same_v<T, bool>) {
        ss << " true or false";
    }

    ss << ", got: " << invalidArg;
    this->m_conversionError = ss.str();
}

template <typename Derived, typename T>
auto Converter<Derived, T>::convert(const std::string& flag, const std::string& value, T& outValue) -> void {
    m_conversionError.clear();
    bool success;
    // Use custom conversion function if supplied
    if (this->m_conversion_fn != nullptr) {
        success = this->m_conversion_fn(value, outValue);
    }
    // Parse as a character
    else if constexpr (is_numeric_char_type<T>) {
        success = parseNumericChar<T>(value, outValue);
    }
    // Parse as a floating point
    else if constexpr (std::is_floating_point_v<T>) {
        success = parseFloatingPoint<T>(value, outValue);
    }
    // Parse as non-bool integral if valid
    else if constexpr (is_non_bool_integral<T>) {
        success = parseIntegralType<T>(value, outValue);
    }
    // Parse as boolean if T is a boolean
    else if constexpr (std::is_same_v<T, bool>) {
        success = parseBool(value, outValue);
    }
    // Use stream extraction if custom conversion not supplied and type is not integral
    else if constexpr (has_stream_extraction<T>::value) {
        std::istringstream iss(value);
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
        generateErrorMsg(flag, value);
    }
}

template <typename Derived, typename T>
auto Converter<Derived, T>::hasConversionError() const -> bool {
    return !m_conversionError.empty();
}

template <typename Derived, typename T>
auto Converter<Derived, T>::getConversionError() -> std::string {
    return m_conversionError;
}

template <typename Derived, typename T>
auto Converter<Derived, T>::withConversionFn(const ConversionFn<T>& conversion_fn) & -> Derived& {
    m_conversion_fn = conversion_fn;
    return static_cast<Derived&>(*this);
}

template <typename Derived, typename T>
auto Converter<Derived, T>::withConversionFn(const ConversionFn<T>& conversion_fn) && -> Derived&& {
    m_conversion_fn = conversion_fn;
    return static_cast<Derived&&>(*this);
}

template <typename Derived, typename T>
auto Converter<Derived, T>::withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) & -> Derived& {
    m_generate_error_msg_fn = generate_error_msg_fn;
    return static_cast<Derived&>(*this);
}

template <typename Derived, typename T>
auto Converter<Derived, T>::withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) && -> Derived&& {
    m_generate_error_msg_fn = generate_error_msg_fn;
    return static_cast<Derived&&>(*this);
}

template<typename T>
Option<T>::Option(T defaultValue) : m_value(defaultValue) {}

template <typename T>
Option<T>::Option(T* out) : OptionBase(), m_out(out) {}

template<typename T>
Option<T>::Option(T defaultValue, T *out) : m_value(defaultValue), m_out(out) {
    *m_out = defaultValue;
}

template<typename T>
auto Option<T>::getValue() const -> const T& {
    return m_value;
}

template <typename T>
auto Option<T>::setValue(const std::string& flag, const std::string& value) -> void {
    T temp;
    this->convert(flag, value, temp);
    if (this->hasConversionError()) {
        this->m_error = this->getConversionError();
        return;
    }
    m_value = temp;
    if (m_out != nullptr) {
        *m_out = temp;
    }
    this->m_isSet = true;
}

inline IOption::IOption(const IOption& other) {
    m_flag          = other.m_flag;
    m_error         = other.m_error;
    m_isSet         = other.m_isSet;
    m_description   = other.m_description;
}

inline auto IOption::operator=(const IOption& other) -> IOption& {
    if (this != &other) {
        m_flag          = other.m_flag;
        m_error         = other.m_error;
        m_isSet         = other.m_isSet;
        m_description   = other.m_description;
    }
    return *this;
}

inline IOption::IOption(IOption&& other) noexcept {
    if (this != &other) {
        m_flag          = std::move(other.m_flag);
        m_error         = std::move(other.m_error);
        m_isSet         = other.m_isSet;
        m_description   = std::move(other.m_description);
    }
}

inline auto IOption::operator=(IOption&& other) noexcept -> IOption& {
    if (this != &other) {
        m_flag          = std::move(other.m_flag);
        m_error         = std::move(other.m_error);
        m_isSet         = other.m_isSet;
        m_description   = std::move(other.m_description);
    }
    return *this;
}

inline auto IOption::applyPrefixes(const std::string_view shortPrefix, const std::string_view longPrefix) -> void {
    m_flag.applyPrefixes(shortPrefix, longPrefix);
}

inline auto IOption::getFlag() const -> const Flag& {
    return m_flag;
}

inline auto IOption::getError() const -> const std::string& {
    return m_error;
}

inline auto IOption::hasError() const -> bool {
    return !m_error.empty();
}

inline auto IOption::getDescription() const -> const std::string& {
    return m_description;
}

inline auto IOption::clearError() -> void {
    m_error.clear();
}

inline auto IOption::isSet() const -> bool {
    return m_isSet;
}

inline OptionGroup::OptionGroup() {
    m_context = std::make_unique<Context>();
}

inline OptionGroup::OptionGroup(const OptionGroup &other) : OptionComponent(other) {
    m_context = std::make_unique<Context>(*other.m_context);
}

inline auto OptionGroup::operator=(const OptionGroup& other) -> OptionGroup& {
    if (this == &other) {
        return *this;
    }

    m_context = std::make_unique<Context>(*other.m_context);
    return *this;
}

template<typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) & -> OptionGroup& {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template <typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::operator+(T&& other) && -> OptionGroup&& {
    m_context->addOption(std::forward<T>(other));
    return std::move(*this);
}

template<typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::addOption(T&& option) -> void {
    m_context->addOption(std::forward<T>(option));
}

inline auto OptionGroup::getOption(const std::string& flag) -> IOption* { //NOLINT (function is not const)
    return m_context->getOption(flag);
}

inline auto OptionGroup::getContext() -> Context& { //NOLINT (function is not const)
    return *m_context;
}

inline auto OptionGroup::getContext() const -> const Context & {
    return *m_context;
}
} // End namespace Argon

#endif // ARGON_OPTION_INCLUDE