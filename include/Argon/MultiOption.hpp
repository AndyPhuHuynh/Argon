#pragma once
#include "Option.hpp"

// Template Specializations

namespace Argon {
    template <typename T>
    class MultiOption;
    
    // MultiOption with C-Style array
    template<typename T>
    class MultiOption<T[]> : public OptionBase, public OptionComponent<MultiOption<T[]>> {
        Converter<T> converter;
        T (*m_out)[];
        size_t m_size;
        size_t m_nextIndex = 0;
        bool m_maxCapacityError = false;
    public:
        MultiOption(T (*out)[]);

        MultiOption(T (*out)[], const ConversionFn<T>& conversion_func);
        
        MultiOption(T (*out)[], const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func);
        
        void set_value(const std::string& flag, const std::string& value) override;
    };
    
    // MultiOption with std::array
    
    template<typename T, size_t N>
    class MultiOption<std::array<T, N>> : public OptionBase, public OptionComponent<MultiOption<std::array<T, N>>> {
        Converter<T> converter;
        std::array<T, N>* m_out;
        size_t m_nextIndex = 0;
        bool m_maxCapacityError = false;
    public:
        MultiOption(std::array<T, N>* out);

        MultiOption(std::array<T, N>* out, const ConversionFn<T>& conversion_func);
        
        MultiOption(std::array<T, N>* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func);

        void set_value(const std::string& flag, const std::string& value) override;
    };

    // MultiOption with std::vector
    template<typename T>
    class MultiOption<std::vector<T>> : public OptionBase, public OptionComponent<MultiOption<std::vector<T>>> {
        Converter<T> converter;
        std::vector<T>* m_out;

    public:
        MultiOption(std::vector<T>* out);

        MultiOption(std::vector<T>* out, const ConversionFn<T>& conversion_func);
        
        MultiOption(std::vector<T>* out, const ConversionFn<T>& conversion_func, const GenerateErrorMsgFn& generate_error_msg_func);

        void set_value(const std::string& flag, const std::string& value) override;
    };
}

// Deduction guides

namespace Argon {
    // Deduction guides for C-Style array

    template <typename T, std::size_t N>
    MultiOption(T (*)[]) -> MultiOption<T[]>;
    
    template <typename T, std::size_t N>
    MultiOption(T (*)[], ConversionFn<T>) -> MultiOption<T[]>;
    
    template <typename T, std::size_t N>
    MultiOption(T (*)[], ConversionFn<T>, GenerateErrorMsgFn) -> MultiOption<T[]>;
    
    // Deduction guides for std::array
    
    template <typename T, std::size_t N>
    MultiOption(std::array<T, N>*) -> MultiOption<std::array<T, N>>;
    
    template <typename T, std::size_t N>
    MultiOption(std::array<T, N>*, ConversionFn<T>) -> MultiOption<std::array<T, N>>;
    
    template <typename T, std::size_t N>
    MultiOption(std::array<T, N>*, ConversionFn<T>, GenerateErrorMsgFn) -> MultiOption<std::array<T, N>>;
    
    // Deduction guides for std::vector

    template <typename T>
    MultiOption(std::vector<T>*) -> MultiOption<std::vector<T>>;

    template <typename T>
    MultiOption(std::vector<T>*, ConversionFn<T>) -> MultiOption<std::vector<T>>;
    
    template <typename T>
    MultiOption(std::vector<T>*, ConversionFn<T>, GenerateErrorMsgFn) -> MultiOption<std::vector<T>>;
}

// Template Implementations

namespace Argon {
    // MultiOption with C-Style array
    
    template <typename T>
    MultiOption<T[]>::MultiOption(T (*out)[]) : m_out(out) {
        m_size = sizeof(*m_out) / sizeof((*m_out)[0]);
    }

    template <typename T>
    MultiOption<T[]>::MultiOption(T (*out)[], const ConversionFn<T>& conversion_func) : m_out(out) {
        m_size = sizeof(*m_out) / sizeof((*m_out)[0]);
        converter.conversion_fn = conversion_func;
    }

    template <typename T>
    MultiOption<T[]>::MultiOption(T (*out)[], const ConversionFn<T>& conversion_func,
        const GenerateErrorMsgFn& generate_error_msg_func) : m_out(out) {
        m_size = sizeof(*m_out) / sizeof((*m_out)[0]);
        converter.conversion_fn = conversion_func;
        converter.generate_error_msg_fn = generate_error_msg_func;
    }

    template <typename T>
    void MultiOption<T[]>::set_value(const std::string& flag, const std::string& value) {
        if (m_maxCapacityError) {
            this->m_error.clear();
            return;
        }
        
        if (m_nextIndex >= m_size) {
            this->m_error = std::format("Flag '{}' only supports a maximum of {} values", flag, m_size);
            m_maxCapacityError = true;
            return;
        }
        
        converter.convert(flag, value, (*m_out)[m_nextIndex]);
        if (converter.has_error()) {
            this->m_error = converter.get_error();
        } else {
            m_nextIndex++;
        }
    }

    // MultiOption with std::array

    template <typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(std::array<T, N>* out) : m_out(out) {}

    template <typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(std::array<T, N>* out,
        const ConversionFn<T>& conversion_func) : m_out(out) {
        converter.conversion_fn = conversion_func;
    }

    template <typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(std::array<T, N>* out,
        const ConversionFn<T>& conversion_func,
        const GenerateErrorMsgFn& generate_error_msg_func) : m_out(out) {
        converter.conversion_fn = conversion_func;
        converter.generate_error_msg_fn = generate_error_msg_func;
    }

    template <typename T, size_t N>
    void MultiOption<std::array<T, N>>::set_value(const std::string& flag, const std::string& value) {
        if (m_maxCapacityError) {
            this->m_error.clear();
            return;
        }
        
        if (m_nextIndex >= N) {
            this->m_error = std::format("Flag '{}' only supports a maximum of {} values", flag, N);
            m_maxCapacityError = true;
            return;
        }
        
        converter.convert(flag, value, (*m_out)[m_nextIndex]);
        if (converter.has_error()) {
            this->m_error = converter.get_error();
        } else {
            m_nextIndex++;
        }
    }

    // MultiOption with std::vector

    template<typename T>
    MultiOption(std::vector<T>) -> MultiOption<std::vector<T>>;
    
    template <typename T>
    MultiOption<std::vector<T>>::MultiOption(std::vector<T>* out) : m_out(out) {}

    template <typename T>
    MultiOption<std::vector<T>>::MultiOption(std::vector<T>* out,
        const ConversionFn<T>& conversion_func) : m_out(out) {
        converter.conversion_fn = conversion_func;
    }

    template <typename T>
    MultiOption<std::vector<T>>::MultiOption(std::vector<T>* out,
        const ConversionFn<T>& conversion_func,
        const GenerateErrorMsgFn& generate_error_msg_func) : m_out(out) {
        converter.conversion_fn = conversion_func;
        converter.generate_error_msg_fn = generate_error_msg_func;
    }

    template <typename T>
    void MultiOption<std::vector<T>>::set_value(const std::string& flag, const std::string& value) {
        T outVal;
        converter.convert(flag, value, outVal);
        if (converter.has_error()) {
            this->m_error = converter.get_error();
        } else {
            m_out->push_back(outVal);
        }
    }
}