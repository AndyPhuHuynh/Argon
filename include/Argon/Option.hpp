#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "Argon.hpp"
#include "StringUtil.h"
#include "Traits.hpp"

namespace Argon {
    class IOption {
    protected:
        std::vector<std::string> m_flags;
        std::string m_usedFlag;
        std::string m_error;
    public:
        IOption() = default;
        virtual ~IOption() = default;
        
        IOption& operator[](const std::string& tag);
        Parser operator| (const IOption& other);

        const std::vector<std::string>& get_flags() const;
        void print_flags() const;
        const std::string& get_error() const;
        bool has_error() const;
        
        virtual IOption* clone() const = 0;
        virtual void set_value(const std::string& flag, const std::string& str) = 0;
        virtual void set_error(const std::string& invalidArg) = 0;
    };
    
    template <typename Derived>
    class OptionBase : public IOption {
    public:
        IOption* clone() const override;
        Derived& operator[](const std::string& tag);
    };
    
    template <typename T>
    class Option final : public OptionBase<Option<T>> {
        T m_value;
        T *m_out = nullptr;
        std::function<bool(const std::string&, T&)> convert = nullptr;
        std::function<std::string(const std::string&, const std::string&)> generate_error_msg = nullptr;
    public:
        explicit Option(T* out);
        
        explicit Option(T* out,
            std::function<bool(const std::string&, T&)> convert);
        
        explicit Option(T* out,
            const std::function<bool(const std::string&, T&)>& convert,
            const std::function<std::string(const std::string&, const std::string&)>& error_msg);

    private:
        void set_value(const std::string& flag, const std::string& arg) override;
        void set_error(const std::string& invalidArg) override;
    };

    template<typename T>
    bool parseIntegralType(const std::string& arg, T& out);
}

// Template Implementations
namespace Argon {
    template <typename Derived>
    IOption* OptionBase<Derived>::clone() const {
        return new Derived(static_cast<const Derived&>(*this));
    }

    template <typename Derived>
    Derived& OptionBase<Derived>::operator[](const std::string& tag) {
        m_flags.push_back(tag);
        return static_cast<Derived&>(*this);
    }

    template <typename T>
    Option<T>::Option(T* out): m_out(out) {
        static_assert(has_stream_extraction<T>::value,
            "Type T must have a conversion function or support stream extraction for parsing.");
    }

    template <typename T>
    Option<T>::Option(T* out, std::function<bool(const std::string&, T&)> convert): m_out(out), convert(convert) {}

    template <typename T>
    Option<T>::Option(T* out,
        const std::function<bool(const std::string&, T&)>& convert,
        const std::function<std::string(const std::string&, const std::string&)>& error_msg)
        : m_out(out), convert(convert), generate_error_msg(error_msg) {}

    template <typename T>
    void Option<T>::set_value(const std::string& flag, const std::string& arg) {
        this->m_usedFlag = flag;
        bool success;
        // Use custom conversion function if supplied
        if (convert != nullptr) {
            success = convert(arg, m_value);
        }
        // Parse as integral if valid
        else if constexpr (std::is_integral_v<T>) {
            success = parseIntegralType<T>(arg, m_value);
        }
        // Use stream extraction if custom conversion not supplied and type is not integral
        else if constexpr (has_stream_extraction<T>::value) {
            std::istringstream iss(arg);
            iss >> m_value;
            success = !iss.fail() && iss.eof();
        }
        // Should never reach this
        else {
            throw std::runtime_error("Type does not support stream extraction,"
                                     "was not an integral type, and no converter was provided.");
        }
        // Set error if not successful
        if (!success) {
            set_error(arg);
        }
        *m_out = m_value;
    }

    template <typename T>
    void Option<T>::set_error(const std::string& invalidArg) {
        // Generate custom error message if provided
        if (generate_error_msg != nullptr) {
            this->m_error = generate_error_msg(this->m_usedFlag, invalidArg);
            return;
        }
        
        // Else generate default message
        std::stringstream ss;
        ss << "Invalid value for flag \'" << this->m_usedFlag << "\': "
            << "expected " << type_name<T>();

        if constexpr (std::is_integral_v<T>) {
            ss << " in the range of [" << StringUtil::format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::min())) <<
                " to " << StringUtil::format_with_commas(static_cast<int64_t>(std::numeric_limits<T>::max())) << "]";
        }

        ss << ", got: " << invalidArg;
        this->m_error = ss.str();
    }

    template <typename T>
    bool parseIntegralType(const std::string& arg, T& out) {
        T min = std::numeric_limits<T>::min();
        T max = std::numeric_limits<T>::max();
        
        try {
            if (std::is_unsigned_v<T>) {
                unsigned long long result = std::stoull(arg);
                if (result > max) {
                    return false;
                }
                out = static_cast<T>(result);
            } else {
                long long result = std::stoll(arg);
                if (result < min || result > max) {
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
