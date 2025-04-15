#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Parser.hpp"
#include "StringUtil.hpp"
#include "Traits.hpp"

namespace Argon {
    class IOption;
    template <typename T> class Option;
    class OptionGroup;
    
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
        
        IOption& operator[](const std::string& tag);
        Parser operator|(const IOption& other);
        operator Parser() const;

        const std::vector<std::string>& get_flags() const;
        void print_flags() const;
        const std::string& get_error() const;
        bool has_error() const;
        
        virtual IOption* clone() const = 0;
    };
    
    template <typename Derived>
    class OptionComponent : public IOption {
        friend Derived;

        OptionComponent() = default;
    public:
        IOption* clone() const override;
        Derived& operator[](const std::string& tag);
    };
    
    class OptionBase {
    public:
        OptionBase() = default;
        virtual ~OptionBase() = default;
        
        virtual void set_value(const std::string& flag, const std::string& value) = 0;
    };
    
    template <typename T>
    class Option : public OptionBase, public OptionComponent<Option<T>> {
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
        void set_error(const std::string& flag, const std::string& invalidArg);
    };

    class OptionGroup : public OptionComponent<OptionGroup> {
        std::vector<IOption*> m_options;
    public:
        OptionGroup& operator+(const IOption& other);

        void add_option(const IOption& option);
        IOption *get_option(const std::string& flag);
        const std::vector<IOption*>& get_options() const;
    };
    
    template<typename T>
    bool parseNonBoolIntegralType(const std::string& arg, T& out);
    bool parseBool(const std::string& arg, bool& out);
}

// Template Implementations

namespace Argon {
    template <typename Derived>
    IOption* OptionComponent<Derived>::clone() const {
        return new Derived(static_cast<const Derived&>(*this));
    }

    template <typename Derived>
    Derived& OptionComponent<Derived>::operator[](const std::string& tag) {
        m_flags.push_back(tag);
        return static_cast<Derived&>(*this);
    }

    template <typename T>
    Option<T>::Option(T* out) : OptionBase(), m_out(out) {
        static_assert(has_stream_extraction<T>::value,
            "Type T must have a conversion function or support stream extraction for parsing.");
    }

    template <typename T>
    Option<T>::Option(T* out, std::function<bool(const std::string&, T&)> convert)
        : OptionBase(), m_out(out), convert(convert) {}

    template <typename T>
    Option<T>::Option(T* out,
        const std::function<bool(const std::string&, T&)>& convert,
        const std::function<std::string(const std::string&, const std::string&)>& error_msg)
        : OptionBase(), m_out(out), convert(convert), generate_error_msg(error_msg) {}

    template <typename T>
    void Option<T>::set_value(const std::string& flag, const std::string& arg) {
        bool success;
        // Use custom conversion function if supplied
        if (convert != nullptr) {
            success = convert(arg, m_value);
        }
        // Parse as non-bool integral if valid
        else if constexpr (is_non_bool_integral<T>) {
            success = parseNonBoolIntegralType<T>(arg, m_value);
        }
        // Parse as boolean if T is a boolean
        else if constexpr (std::is_same_v<T, bool>) {
            success = parseBool(arg, m_value);
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
            set_error(flag, arg);
        }
        *m_out = m_value;
    }

    template <typename T>
    void Option<T>::set_error(const std::string& flag, const std::string& invalidArg) {
        // Generate custom error message if provided
        if (generate_error_msg != nullptr) {
            this->m_error = generate_error_msg(flag, invalidArg);
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
