#ifndef ARGON_OPTION_INCLUDE
#define ARGON_OPTION_INCLUDE

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <typeindex>
#include <vector>

#include "Flag.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename T> class Option;
    class OptionGroup;
    class Context;

    using DefaultConversionFn = std::function<bool(std::string_view, void*)>;
    using DefaultConversions  = std::unordered_map<std::type_index, DefaultConversionFn>;

    class IFlag {
    protected:
        Flag m_flag;
    public:
        IFlag() = default;
        ~IFlag() = default;

        IFlag(const IFlag&) = default;
        IFlag& operator=(const IFlag&) = default;

        IFlag(IFlag&&) = default;
        IFlag& operator=(IFlag&&) = default;

        [[nodiscard]] auto getFlag() const -> const Flag&;

        auto applyPrefixes(std::string_view shortPrefix, std::string_view longPrefix) -> void;
    };

    template <typename Derived>
    class HasFlag : public IFlag{
        auto applySetFlag(std::string_view flag) -> void;
        auto applySetFlag(std::initializer_list<std::string_view> flags) -> void;
    public:
        auto operator[](std::string_view flag) & -> Derived&;
        auto operator[](std::string_view flag) && -> Derived&&;

        auto operator[](std::initializer_list<std::string_view> flags) & -> Derived&;
        auto operator[](std::initializer_list<std::string_view> flags) && -> Derived&&;
    };

    class IOption {
    protected:
        std::string m_error;
        std::string m_inputHint;
        std::string m_description;

        bool m_isSet = false;
        IOption() = default;
    public:
        IOption(const IOption&);
        IOption& operator=(const IOption&);
        IOption(IOption&&) noexcept;
        IOption& operator=(IOption&&) noexcept;
        virtual ~IOption() = default;

        [[nodiscard]] auto getError() const -> const std::string&;

        [[nodiscard]] auto hasError() const -> bool;

        [[nodiscard]] auto getInputHint() const -> const std::string&;

        [[nodiscard]] auto getDescription() const -> const std::string&;

        auto clearError() -> void;

        [[nodiscard]] auto isSet() const -> bool;

        [[nodiscard]] virtual auto clone() const -> std::unique_ptr<IOption> = 0;
    };

    template <typename Derived>
    class OptionComponent : public IOption {
        friend Derived;

        OptionComponent() = default;
    public:
        [[nodiscard]] std::unique_ptr<IOption> clone() const override;

        auto description(std::string_view desc) & -> Derived&;
        auto description(std::string_view desc) && -> Derived&&;
        auto description(std::string_view inputHint, std::string_view desc) & -> Derived&;
        auto description(std::string_view inputHint, std::string_view desc) && -> Derived&&;

        auto operator()(std::string_view desc) & -> Derived&;
        auto operator()(std::string_view desc) && -> Derived&&;
        auto operator()(std::string_view inputHint, std::string_view desc) & -> Derived&;
        auto operator()(std::string_view inputHint, std::string_view desc) && -> Derived&&;
    };

    class ISetValue {
    public:
        ISetValue() = default;
        virtual ~ISetValue() = default;

        virtual void setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) = 0;
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

        auto generateErrorMsg(std::string_view optionName, std::string_view invalidArg) -> void;

    public:
        auto convert(const DefaultConversions& conversions, std::string_view flag, std::string_view value, T& outValue) -> void;

        [[nodiscard]] auto hasConversionError() const -> bool;

        auto getConversionError() -> std::string;

        auto withConversionFn(const ConversionFn<T>& conversion_fn) & -> Derived&;

        auto withConversionFn(const ConversionFn<T>& conversion_fn) && -> Derived&&;

        auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) & -> Derived&;

        auto withErrorMsgFn(const GenerateErrorMsgFn& generate_error_msg_fn) && -> Derived&&;
    };

    template <typename Derived, typename T>
    class SetValueImpl : public ISetValue, public Converter<Derived, T> {
        T m_value = T();
        T *m_out = nullptr;
    public:
        SetValueImpl() = default;

        explicit SetValueImpl(T defaultValue);

        explicit SetValueImpl(T *out);

        SetValueImpl(T defaultValue, T *out);

        auto getValue() const -> const T&;
    protected:
        auto setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) -> void override;
    };

    class IsSingleOption {};

    template <typename T>
    class Option : public HasFlag<Option<T>>, public SetValueImpl<Option<T>, T>,
                   public OptionComponent<Option<T>>, public IsSingleOption {
    public:
        Option() = default;

        explicit Option(T defaultValue);

        explicit Option(T *out);

        Option(T defaultValue, T *out);
    protected:
        auto setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) -> void override;
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
        auto operator+(T&& other) && -> OptionGroup&&;

        template <typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto getOption(std::string_view flag) -> IOption*;

        auto getContext() -> Context&;

        [[nodiscard]] auto getContext() const -> const Context&;
    };

    class IsPositional {
    public:
        IsPositional() = default;
        virtual ~IsPositional() = default;

        [[nodiscard]] virtual auto cloneAsPositional() const -> std::unique_ptr<IsPositional> = 0;
    };

    template <typename T>
    class Positional : public SetValueImpl<Positional<T>, T>,
                       public OptionComponent<Positional<T>>, public IsPositional {
        auto setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) -> void override;
    public:
        Positional() = default;

        explicit Positional(T defaultValue);

        explicit Positional(T *out);

        Positional(T defaultValue, T *out);

        [[nodiscard]] auto cloneAsPositional() const -> std::unique_ptr<IsPositional> override;
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
auto parseIntegralType(const std::string_view arg, T& out) -> bool {
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

inline auto parseBool(const std::string_view arg, bool& out) -> bool {
    auto boolStr = std::string(arg);
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
auto parseNumericChar(const std::string_view arg, T& out) -> bool {
    if (arg.length() == 1) {
        out = static_cast<T>(arg[0]);
        return true;
    }
    return parseIntegralType(arg, out);
}

template <typename T> requires std::is_floating_point_v<T>
auto parseFloatingPoint(const std::string_view arg, T& out) -> bool {
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
inline auto IFlag::getFlag() const -> const Flag& {
    return m_flag;
}

inline auto IFlag::applyPrefixes(const std::string_view shortPrefix, const std::string_view longPrefix) -> void {
    m_flag.applyPrefixes(shortPrefix, longPrefix);
}

template<typename Derived>
auto HasFlag<Derived>::applySetFlag(std::string_view flag) -> void {
    if (flag.empty()) {
        throw std::invalid_argument("Flag has to be at least one character long");
    }
    if (m_flag.mainFlag.empty()) {
        m_flag.mainFlag = flag;
    } else {
        m_flag.aliases.emplace_back(flag);
    }
}

template<typename Derived>
auto HasFlag<Derived>::applySetFlag(const std::initializer_list<std::string_view> flags) -> void {
    if (flags.size() <= 0) {
        throw std::invalid_argument("Operator [] expects at least one flag");
    }
    for (const auto& flag : flags) {
        applySetFlag(flag);
    }
}

template <typename Derived>
auto HasFlag<Derived>::operator[](const std::string_view flag) & -> Derived& {
    applySetFlag(flag);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::string_view flag) && -> Derived&& {
    applySetFlag(flag);
    return static_cast<Derived&&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::initializer_list<std::string_view> flags) & -> Derived& {
    applySetFlag(flags);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto HasFlag<Derived>::operator[](const std::initializer_list<std::string_view> flags) && -> Derived&& {
    applySetFlag(flags);
    return static_cast<Derived&&>(*this);
}

template <typename Derived>
auto OptionComponent<Derived>::clone() const -> std::unique_ptr<IOption> {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
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

template<typename Derived>
auto OptionComponent<Derived>::description(const std::string_view inputHint, const std::string_view desc) & -> Derived& {
    m_inputHint = inputHint;
    m_description = desc;
    return static_cast<Derived&>(*this);
}

template<typename Derived>
auto OptionComponent<Derived>::description(const std::string_view inputHint, const std::string_view desc) && -> Derived&& {
    m_inputHint = inputHint;
    m_description = desc;
    return static_cast<Derived&&>(*this);
}

template<typename Derived>
auto OptionComponent<Derived>::operator()(const std::string_view desc) & -> Derived& {
    return static_cast<Derived&>(description(desc));
}

template<typename Derived>
auto OptionComponent<Derived>::operator()(const std::string_view desc) && -> Derived&& {
    return static_cast<Derived&&>(description(desc));
}

template<typename Derived>
auto OptionComponent<Derived>::operator()(const std::string_view inputHint, const std::string_view desc) & -> Derived& {
    return static_cast<Derived&>(description(inputHint, desc));
}

template<typename Derived>
auto OptionComponent<Derived>::operator()(const std::string_view inputHint, const std::string_view desc) && -> Derived&& {
    return static_cast<Derived&&>(description(inputHint, desc));
}

template <typename Derived, typename T>
auto Converter<Derived, T>::generateErrorMsg(std::string_view optionName, std::string_view invalidArg) -> void {
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
        ss << std::format("Invalid value for '{}': ", optionName);
    }
    ss << "expected " << type_name<T>();

    if constexpr (is_non_bool_integral<T>) {
        ss << std::format(
            " between {} and {}",
            format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::min())),
            format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::max())));
    } else if constexpr (std::is_same_v<T, bool>) {
        ss << " (true or false)";
    }
    ss << std::format(", got: '{}'", invalidArg);
    this->m_conversionError = ss.str();
}

template <typename Derived, typename T>
auto Converter<Derived, T>::convert(const DefaultConversions& conversions,
                                    const std::string_view flag, std::string_view value, T& outValue) -> void {
    m_conversionError.clear();
    bool success;
    // Use custom conversion function for this specific option if supplied
    if (this->m_conversion_fn != nullptr) {
        success = this->m_conversion_fn(value, outValue);
    }
    // Search for conversion list for conversion for this type if specified
    else if (conversions.contains(std::type_index(typeid(T)))) {
        success = conversions.at(std::type_index(typeid(T)))(value, static_cast<void*>(&outValue));
    }
    // Fallback to generic parsing
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

template<typename Derived, typename T>
SetValueImpl<Derived, T>::SetValueImpl(T defaultValue) : m_value(defaultValue) {}

template<typename Derived, typename T>
SetValueImpl<Derived, T>::SetValueImpl(T* out) : ISetValue(), m_out(out) {}

template<typename Derived, typename T>
SetValueImpl<Derived, T>::SetValueImpl(T defaultValue, T *out) : m_value(defaultValue), m_out(out) {
    *m_out = defaultValue;
}

template<typename Derived, typename T>
auto SetValueImpl<Derived, T>::getValue() const -> const T& {
    return m_value;
}

template<typename Derived, typename T>
auto SetValueImpl<Derived, T>::setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) -> void {
    T temp;
    this->convert(conversions, flag, value, temp);
    if (this->hasConversionError()) {
        return;
    }
    m_value = temp;
    if (m_out != nullptr) {
        *m_out = temp;
    }
}

template<typename T>
Option<T>::Option(T defaultValue) : SetValueImpl<Option, T>(defaultValue) {}

template<typename T>
Option<T>::Option(T *out) : SetValueImpl<Option, T>(out) {}

template<typename T>
Option<T>::Option(T defaultValue, T *out) : SetValueImpl<Option, T>(defaultValue, out) {}

template<typename T>
void Option<T>::setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) {
    SetValueImpl<Option, T>::setValue(conversions, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

inline IOption::IOption(const IOption& other) {
    m_error         = other.m_error;
    m_isSet         = other.m_isSet;
    m_inputHint      = other.m_inputHint;
    m_description   = other.m_description;
}

inline auto IOption::operator=(const IOption& other) -> IOption& {
    if (this != &other) {
        m_error         = other.m_error;
        m_isSet         = other.m_isSet;
        m_inputHint      = other.m_inputHint;
        m_description   = other.m_description;
    }
    return *this;
}

inline IOption::IOption(IOption&& other) noexcept {
    if (this != &other) {
        m_error         = std::move(other.m_error);
        m_isSet         = other.m_isSet;
        m_inputHint      = std::move(other.m_inputHint);
        m_description   = std::move(other.m_description);
    }
}

inline auto IOption::operator=(IOption&& other) noexcept -> IOption& {
    if (this != &other) {
        m_error         = std::move(other.m_error);
        m_isSet         = other.m_isSet;
        m_inputHint      = std::move(other.m_inputHint);
        m_description   = std::move(other.m_description);
    }
    return *this;
}

inline auto IOption::getError() const -> const std::string& {
    return m_error;
}

inline auto IOption::hasError() const -> bool {
    return !m_error.empty();
}

inline auto IOption::getInputHint() const -> const std::string& {
    return m_inputHint;
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
auto OptionGroup::operator+(T&& other) && -> OptionGroup&& {
    m_context->addOption(std::forward<T>(other));
    return std::move(*this);
}

template<typename T> requires DerivesFrom<T, IOption>
auto OptionGroup::addOption(T&& option) -> void {
    m_context->addOption(std::forward<T>(option));
}

template<typename T>
void Positional<T>::setValue(const DefaultConversions& conversions, std::string_view flag, std::string_view value) {
    SetValueImpl<Positional, T>::setValue(conversions, flag, value);
    this->m_error = this->getConversionError();
    this->m_isSet = true;
}

template<typename T>
Positional<T>::Positional(T defaultValue) : SetValueImpl<Positional, T>(defaultValue) {}

template<typename T>
Positional<T>::Positional(T *out) : SetValueImpl<Positional, T>(out) {}

template<typename T>
Positional<T>::Positional(T defaultValue, T *out) : SetValueImpl<Positional, T>(defaultValue, out) {}

template<typename T>
auto Positional<T>::cloneAsPositional() const -> std::unique_ptr<IsPositional> {
    return std::make_unique<Positional>(*this);
}

inline auto OptionGroup::getOption(std::string_view flag) -> IOption* { //NOLINT (function is not const)
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