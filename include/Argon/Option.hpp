#pragma once
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "Argon.hpp"

namespace Argon {
    class IOption {
    public:
        std::vector<std::string> tags;
    public:
        IOption() = default;
        virtual ~IOption() = default;
        
        
        IOption& operator[](const std::string& tag);
        Parser operator| (const IOption& other);

        void printTags() const;
        
        virtual std::shared_ptr<IOption> clone() const = 0;
        virtual void setValue(const std::string& str) = 0;
    };
    
    template <typename Derived>
    class OptionBase : public IOption {
    public:
        std::shared_ptr<IOption> clone() const override;
        Derived& operator[](const std::string& tag);
    };
    
    template <typename T>
    class Option final : public OptionBase<Option<T>> {
        T *out;
    public:
        explicit Option(T* out) : out(out) {}
    private:
        void setValue(const std::string& str) override;
    };
}

// Template Implementations
namespace Argon {
    template <typename Derived>
    std::shared_ptr<IOption> OptionBase<Derived>::clone() const {
        return std::make_shared<Derived>(static_cast<const Derived&>(*this));
    }

    template <typename Derived>
    Derived& OptionBase<Derived>::operator[](const std::string& tag) {
        tags.push_back(tag);
        return static_cast<Derived&>(*this);
    }

    template <typename T>
    void Option<T>::setValue(const std::string& str) {
        std::istringstream iss(str);
        iss >> *out;
        if (iss.fail() || !iss.eof()) {
            std::cerr << "Invalid option value: " << str << "\n";
        }
    }
}