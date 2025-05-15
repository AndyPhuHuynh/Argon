#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

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
        Derived& operator[](const std::string& tag);
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
        T *m_out = nullptr;
    public:
        explicit Option(T* out);
        
        Option(T* out, const ConversionFn<T>& conversion_func);
        
        Option(T* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func);

    private:
        void set_value(const std::string& flag, const std::string& value) override;
    };

    class OptionGroup : public OptionComponent<OptionGroup> {
        std::shared_ptr<Context> m_context;
    public:
        OptionGroup();
        OptionGroup& operator+(const IOption& other);

        void add_option(const IOption& option);
        IOption *get_option(const std::string& flag);
        Context& get_context();
    };
    
    template<typename T>
    bool parseNonBoolIntegralType(const std::string& arg, T& out);
    bool parseBool(const std::string& arg, bool& out);
}


#include "Option.tpp"
