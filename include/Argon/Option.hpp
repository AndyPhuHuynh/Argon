#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename T> class Option;
    class OptionGroup;
    class Context;

    class IOption {
    protected:
        std::vector<std::string> m_flags;
        std::string m_error;

        IOption() = default;
    public:
        IOption(const IOption&);
        IOption& operator=(const IOption&);
        IOption(IOption&&) noexcept;
        IOption& operator=(IOption&&) noexcept;
        virtual ~IOption() = default;

        [[nodiscard]] const std::vector<std::string>& getFlags() const;
        void printFlags() const;
        [[nodiscard]] const std::string& getError() const;
        [[nodiscard]] bool hasError() const;
        void clearError();

        [[nodiscard]] virtual std::unique_ptr<IOption> clone() const = 0;
    };

    template <typename Derived>
    class OptionComponent : public IOption {
        friend Derived;

        OptionComponent() = default;
    public:
        [[nodiscard]] std::unique_ptr<IOption> clone() const override;
        Derived& operator[](const std::string& tag) &;
        Derived&& operator[](const std::string& tag) &&;
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

    template <typename T>
    struct Converter {
        ConversionFn<T> conversion_fn = nullptr;
        GenerateErrorMsgFn generate_error_msg_fn = nullptr;
    private:
        std::string m_error;
        void generateErrorMsg(const std::string& flag, const std::string& invalidArg);
    public:
        void convert(const std::string& flag, const std::string& value, T& outValue);
        [[nodiscard]] bool hasError() const;
        std::string getError();
    };

    class IsSingleOption {};

    template <typename T>
    class Option : public OptionBase, public OptionComponent<Option<T>>, public IsSingleOption {
        Converter<T> converter;
        std::optional<T> m_value;
        T *m_out = nullptr;
    public:
        explicit Option(T* out);

        Option(T* out, const ConversionFn<T>& conversion_func);

        Option(T* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func);

        const std::optional<T>& getValue() const;
    private:
        void setValue(const std::string& flag, const std::string& value) override;
    };

    class OptionGroup : public OptionComponent<OptionGroup> {
        std::unique_ptr<Context> m_context;
    public:
        OptionGroup();
        OptionGroup(const OptionGroup&);
        OptionGroup& operator=(const OptionGroup&);

        template <typename T> requires DerivesFrom<T, IOption>
        OptionGroup& operator+(T&& other) &;

        template <typename T> requires DerivesFrom<T, IOption>
        OptionGroup&& operator+(T&& other) &&;

        template <typename T> requires DerivesFrom<T, IOption>
        void addOption(T&& option);

        IOption *getOption(const std::string& flag);
        Context& getContext();
    };
}

//------------------------------------------------------Includes--------------------------------------------------------

#include "Context.hpp"
#include "StringUtil.hpp"

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
enum class Base {
    Invalid = 0,
    Binary = 2,
    Octal = 8,
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
    if (arg[baseIndex] == 'o' || arg[baseIndex] == 'O')         return Base::Octal;
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
    StringUtil::to_lower(boolStr);
    if (boolStr == "true") {
        out = true;
        return true;
    } else if (boolStr == "false") {
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

template <typename Derived>
std::unique_ptr<Argon::IOption> Argon::OptionComponent<Derived>::clone() const {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
}

template <typename Derived>
Derived& Argon::OptionComponent<Derived>::operator[](const std::string& tag) & {
    m_flags.push_back(tag);
    return static_cast<Derived&>(*this);
}

template<typename Derived>
Derived&& Argon::OptionComponent<Derived>::operator[](const std::string& tag) && {
    m_flags.push_back(tag);
    return static_cast<Derived&&>(*this);
}

template <typename T>
void Argon::Converter<T>::generateErrorMsg(const std::string& flag, const std::string& invalidArg) {
    // Generate custom error message if provided
    if (this->generate_error_msg_fn != nullptr) {
        this->m_error = this->generate_error_msg_fn(flag, invalidArg);
        return;
    }

    // Else generate default message
    std::stringstream ss;
    ss << "Invalid value for flag \'" << flag << "\': "
        << "expected " << type_name<T>();

    if constexpr (is_non_bool_integral<T>) {
        ss << " in the range of [" << StringUtil::format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::min())) <<
              " to " << StringUtil::format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::max())) << "]";
    } else if constexpr (std::is_same_v<T, bool>) {
        ss << " true or false";
    }

    ss << ", got: " << invalidArg;
    this->m_error = ss.str();
}

template <typename T>
void Argon::Converter<T>::convert(const std::string& flag, const std::string& value, T& outValue) {
    m_error.clear();
    bool success;
    // Use custom conversion function if supplied
    if (this->conversion_fn != nullptr) {
        success = this->conversion_fn(value, outValue);
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
        throw std::runtime_error("Type does not support stream extraction,"
                                 "was not an integral type, and no converter was provided.");
    }
    // Set error if not successful
    if (!success) {
        generateErrorMsg(flag, value);
    }
}

template <typename T>
bool Argon::Converter<T>::hasError() const {
    return !m_error.empty();
}

template <typename T>
std::string Argon::Converter<T>::getError() {
    return m_error;
}

template <typename T>
Argon::Option<T>::Option(T* out) : OptionBase(), m_out(out) {
    static_assert(has_stream_extraction<T>::value,
        "Type T must have a conversion function or support stream extraction for parsing.");
}

template <typename T>
Argon::Option<T>::Option(T* out, const ConversionFn<T>& conversion_func)
    : OptionBase(), m_out(out) {
    converter.conversion_fn = conversion_func;
}

template <typename T>
Argon::Option<T>::Option(T* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func)
    : OptionBase(), m_out(out) {
    converter.conversion_fn = conversion_func;
    converter.generate_error_msg_fn = generate_error_msg_func;
}

template<typename T>
const std::optional<T>& Argon::Option<T>::getValue() const {
    return m_value;
}

template <typename T>
void Argon::Option<T>::setValue(const std::string& flag, const std::string& value) {
    T temp;
    converter.convert(flag, value, temp);
    if (converter.hasError()) {
        this->m_error = converter.getError();
        return;
    }
    m_value = temp;
    if (m_out != nullptr) {
        *m_out = temp;
    }
}

inline Argon::IOption::IOption(const IOption& other) {
    m_flags = other.m_flags;
    m_error = other.m_error;
}

inline Argon::IOption& Argon::IOption::operator=(const IOption& other) {
    if (this != &other) {
        m_flags = other.m_flags;
        m_error = other.m_error;
    }
    return *this;
}

inline Argon::IOption::IOption(IOption&& other) noexcept {
    if (this != &other) {
        m_flags = std::move(other.m_flags);
        m_error = std::move(other.m_error);
    }
}

inline Argon::IOption& Argon::IOption::operator=(IOption&& other) noexcept {
    if (this != &other) {
        m_flags = std::move(other.m_flags);
        m_error = std::move(other.m_error);
    }
    return *this;
}

inline const std::vector<std::string>& Argon::IOption::getFlags() const {
    return m_flags;
}

inline void Argon::IOption::printFlags() const {
    for (auto& tag : m_flags) {
        std::cout << tag << "\n";
    }
}

inline const std::string& Argon::IOption::getError() const {
    return m_error;
}

inline auto Argon::IOption::hasError() const -> bool {
    return !m_error.empty();
}

inline void Argon::IOption::clearError() {
    m_error.clear();
}

inline Argon::OptionGroup::OptionGroup() {
    m_context = std::make_unique<Context>();
}

inline Argon::OptionGroup::OptionGroup(const OptionGroup &other) : OptionComponent(other) {
    m_context = std::make_unique<Context>(*other.m_context);
}

inline Argon::OptionGroup & Argon::OptionGroup::operator=(const OptionGroup &other) {
    if (this == &other) {
        return *this;
    }

    m_context = std::make_unique<Context>(*other.m_context);
    return *this;
}

template<typename T> requires Argon::DerivesFrom<T, Argon::IOption>
Argon::OptionGroup& Argon::OptionGroup::operator+(T&& other) & {
    m_context->addOption(std::forward<T>(other));
    return *this;
}

template <typename T> requires Argon::DerivesFrom<T, Argon::IOption>
Argon::OptionGroup&& Argon::OptionGroup::operator+(T&& other) && {
    m_context->addOption(std::forward<T>(other));
    return std::move(*this);
}

template<typename T> requires Argon::DerivesFrom<T, Argon::IOption>
void Argon::OptionGroup::addOption(T&& option) {
    m_context->addOption(std::forward<T>(option));
}

inline Argon::IOption *Argon::OptionGroup::getOption(const std::string& flag) { //NOLINT (function is not const)
    return m_context->getOption(flag);
}

inline Argon::Context& Argon::OptionGroup::getContext() { //NOLINT (function is not const)
    return *m_context;
}
