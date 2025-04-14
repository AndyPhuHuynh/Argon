#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Ast.hpp"
#include "Scanner.hpp"

namespace Argon {
    class IOption;
    template<typename T>
    class Option;
    
    class Parser {
    public:
        std::vector<IOption*> options;

    private:
        Scanner scanner = Scanner();
    public:
        Parser() = default;
        Parser(const Parser& parser);
        Parser(Parser&& parser) noexcept;
        Parser& operator=(const Parser& parser);
        Parser& operator=(Parser&& parser) noexcept;
        ~Parser();
        
        void addOption(const IOption& option);
        IOption *getOption(const std::string& flag);
        static IOption *getOption(const std::vector<IOption*>& options, const std::string& flag);
        void parseString(const std::string& str);

        StatementAst parseStatement();
        std::unique_ptr<OptionAst> parseOption();
        std::unique_ptr<OptionGroupAst> parseOptionGroup();
        
        Parser& operator|(const IOption& option);
    };
}
