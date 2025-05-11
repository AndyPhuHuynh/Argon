#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Argon {
    class IOption;
    
    class Context {
        std::vector<std::unique_ptr<IOption>> m_options;
        std::vector<std::string> m_flags;

    public:
        Context() = default;
        Context(const Context&);
        Context& operator=(const Context&);
        
        void addOption(const IOption& option);
        IOption *getOption(const std::string& flag);

        template <typename T>
        T *getOptionDynamic(const std::string& flag);
        
        bool containsLocalFlag(const std::string& flag) const;
    };
}

// Template Implementations

namespace Argon {
    template <typename T>
    T* Context::getOptionDynamic(const std::string& flag) {
        return dynamic_cast<T*>(getOption(flag));
    }
}