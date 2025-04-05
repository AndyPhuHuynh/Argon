#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Argon {
    class IOption;
    template<typename T>
    class Option;
    
    class Parser {
    public:
        std::vector<std::shared_ptr<IOption>> options;

        void addOption(const IOption& option);
        void addOption(const std::shared_ptr<IOption>& option);
        template<typename T>
        void addOption(const Option<T>& option) {
            options.push_back(std::make_shared<Option<T>>(option));
        }
        bool tokenInOptions(const std::string& token);
        IOption& searchOptions(const std::string& token);
        void parseString(const std::string& str);

        Parser& operator|(const IOption& option);

        template<typename T>
        Parser& operator|(const Option<T>& option) {
            options.emplace_back(std::make_shared<Option<T>>(option));
            return *this;
        }
    };
}
