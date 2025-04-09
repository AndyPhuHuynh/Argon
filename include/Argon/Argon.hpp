#pragma once

#include <string>
#include <vector>

#include "Scanner.hpp"

namespace Argon {
    class IOption;
    template<typename T>
    class Option;
    
    class Parser {
    public:
        std::vector<IOption*> options;

    private:
        Scanner scanner;
    public:
        Parser() = default;
        Parser(const Parser& parser);
        Parser(Parser&& parser) noexcept;
        Parser& operator=(const Parser& parser);
        Parser& operator=(Parser&& parser) noexcept;
        ~Parser();
        
        void addOption(const IOption& option);
        bool getOption(const std::string& token, IOption*& out);
        void parseString(const std::string& str);

        void parseStatement();
        void parseOption();
        void parseOptionGroup();

        Parser& operator|(const IOption& option);
    };
}
