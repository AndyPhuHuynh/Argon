#pragma once
#include "Argon/Option.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Context.hpp"
#include "StringUtil.hpp"
#include "Traits.hpp"

// Template Implementations

namespace Argon {
    template <typename Derived>
    std::unique_ptr<IOption> OptionComponent<Derived>::clone() const {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }

    template <typename Derived>
    Derived& OptionComponent<Derived>::operator[](const std::string& tag) {
        m_flags.push_back(tag);
        return static_cast<Derived&>(*this);
    }

    template <typename T>
    void Converter<T>::generate_error_msg(const std::string& flag, const std::string& invalidArg) {
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
    void Converter<T>::convert(const std::string& flag, const std::string& value, T& outValue) {
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
    bool Converter<T>::has_error() const {
        return !m_error.empty();
    }

    template <typename T>
    std::string Converter<T>::get_error() {
        return m_error;
    }

    template <typename T>
    Option<T>::Option(T* out) : OptionBase(), m_out(out) {
        static_assert(has_stream_extraction<T>::value,
            "Type T must have a conversion function or support stream extraction for parsing.");
    }

    template <typename T>
    Option<T>::Option(T* out, const ConversionFn<T>& conversion_func)
        : OptionBase(), m_out(out) {
        converter.conversion_fn = conversion_func;
    }

    template <typename T>
    Option<T>::Option(T* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func)
        : OptionBase(), m_out(out) {
        converter.conversion_fn = conversion_func;
        converter.generate_error_msg_fn = generate_error_msg_func;
    }

    template <typename T>
    void Option<T>::set_value(const std::string& flag, const std::string& value) {
        converter.convert(flag, value, *m_out);
        if (converter.has_error()) {
            this->m_error = converter.get_error();
        }
    }

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
    m_context = std::make_shared<Context>();
}

inline Argon::OptionGroup& Argon::OptionGroup::operator+(const IOption& other) {
    add_option(other);
    return *this;
}

inline void Argon::OptionGroup::add_option(const IOption& option) {
    m_context->addOption(option);
}

inline Argon::IOption *Argon::OptionGroup::get_option(const std::string& flag) {
    return m_context->getOption(flag);
}

inline Argon::Context& Argon::OptionGroup::get_context() {
    return *m_context;
}

inline bool Argon::parseBool(const std::string& arg, bool& out) {
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
