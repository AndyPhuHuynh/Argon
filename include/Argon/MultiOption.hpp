#pragma once
#include "Option.hpp"

namespace Argon {
    class MultiOptionBase {
        MultiOptionBase() = default;
        virtual ~MultiOptionBase() = default;
        
        virtual bool set_value(const std::string& flag, const std::string& value) = 0;
    };
    
    template<typename T>
    class MultiOption : public IOption, public OptionComponent<MultiOption<T>> {
        
    };
}

// Template Specializations

namespace Argon {
    // MultiOption with C-Style array
    template<typename T>
    class MultiOption<T[]> {
        T (*m_out)[];

    public:
        MultiOption(T (*out)[]); 
    };

    // MultiOption with std::array
    template<typename T, size_t N>
    class MultiOption<std::array<T, N>> {
        std::array<T, N>* m_out;

    public:
        MultiOption(std::vector<T>* out); 
    };

    // MultiOption with std::vector
    template<typename T>
    class MultiOption<std::vector<T>> {
        std::vector<T>* m_out;

    public:
        MultiOption(std::vector<T>* out); 
    };
}

// Template Implementations
namespace Argon {
    
}