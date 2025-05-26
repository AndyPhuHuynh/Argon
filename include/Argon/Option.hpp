#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <optional>
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

        [[nodiscard]] const std::vector<std::string>& get_flags() const;
        void print_flags() const;
        [[nodiscard]] const std::string& get_error() const;
        [[nodiscard]] bool has_error() const;
        void clear_error();

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

        virtual void set_value(const std::string& flag, const std::string& value) = 0;
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
        void generate_error_msg(const std::string& flag, const std::string& invalidArg);
    public:
        void convert(const std::string& flag, const std::string& value, T& outValue);
        [[nodiscard]] bool has_error() const;
        std::string get_error();
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

        std::optional<T> get_value();
    private:
        void set_value(const std::string& flag, const std::string& value) override;
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
        void add_option(T&& option);

        IOption *get_option(const std::string& flag);
        Context& get_context();
    };
}

//------------------------------------------------------Includes--------------------------------------------------------

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Context.hpp"
#include "StringUtil.hpp"

//---------------------------------------------------Free Functions-----------------------------------------------------

namespace Argon {
template <typename T>
bool parseNonBoolIntegralType(const std::string& arg, T& out) {
    static_assert(is_non_bool_integral<T>);
    T min = std::numeric_limits<T>::min();
    T max = std::numeric_limits<T>::max();

    try {
        if (std::is_unsigned_v<T>) {
            size_t pos;
            unsigned long long result = std::stoull(arg, &pos);
            if (result > max || pos != arg.size()) {
                return false;
            }
            out = static_cast<T>(result);
        } else {
            size_t pos;
            long long result = std::stoll(arg, &pos);
            if (result < min || result > max || pos != arg.size()) {
                return false;
            }
            out = static_cast<T>(result);
        }
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

inline bool parseBool(const std::string& arg, bool& out) {
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
}

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
void Argon::Converter<T>::generate_error_msg(const std::string& flag, const std::string& invalidArg) {
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
    // Parse as non-bool integral if valid
    else if constexpr (is_non_bool_integral<T>) {
        success = parseNonBoolIntegralType<T>(value, outValue);
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
        generate_error_msg(flag, value);
    }
}

template <typename T>
bool Argon::Converter<T>::has_error() const {
    return !m_error.empty();
}

template <typename T>
std::string Argon::Converter<T>::get_error() {
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
std::optional<T> Argon::Option<T>::get_value() {
    return m_value;
}

template <typename T>
void Argon::Option<T>::set_value(const std::string& flag, const std::string& value) {
    T temp;
    converter.convert(flag, value, temp);
    if (converter.has_error()) {
        this->m_error = converter.get_error();
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

inline const std::vector<std::string>& Argon::IOption::get_flags() const {
    return m_flags;
}

inline void Argon::IOption::print_flags() const {
    for (auto& tag : m_flags) {
        std::cout << tag << "\n";
    }
}

inline const std::string& Argon::IOption::get_error() const {
    return m_error;
}

inline auto Argon::IOption::has_error() const -> bool {
    return !m_error.empty();
}

inline void Argon::IOption::clear_error() {
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
void Argon::OptionGroup::add_option(T&& option) {
    m_context->addOption(std::forward<T>(option));
}

inline Argon::IOption *Argon::OptionGroup::get_option(const std::string& flag) { //NOLINT (function is not const)
    return m_context->getOption(flag);
}

inline Argon::Context& Argon::OptionGroup::get_context() { //NOLINT (function is not const)
    return *m_context;
}
