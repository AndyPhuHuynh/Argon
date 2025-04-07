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
        std::vector<IOption*> options;

        Parser() = default;
        Parser(const Parser& parser);
        Parser(Parser&& parser) noexcept;
        Parser& operator=(const Parser& parser);
        Parser& operator=(Parser&& parser) noexcept;
        ~Parser();
        
        void addOption(const IOption& option);
        bool getOption(const std::string& token, IOption*& out);
        void parseString(const std::string& str);

        Parser& operator|(const IOption& option);
    };
}
