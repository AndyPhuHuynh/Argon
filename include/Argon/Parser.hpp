#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Ast.hpp"
#include "Error.hpp"
#include "Scanner.hpp"

namespace Argon {
    class IOption;
    template<typename T>
    class Option;
    
    class Parser {
        std::vector<IOption*> m_options;
        Scanner m_scanner = Scanner();

        ErrorGroup m_rootErrorGroup = ErrorGroup("Top-Level", -1, -1);
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

        void addError(const std::string& error, int pos);
        void addErrorGroup(const std::string& groupName, int startPos, int endPos);
        
        void parseString(const std::string& str);
        StatementAst parseStatement();
        std::unique_ptr<OptionAst> parseOption();
        std::unique_ptr<OptionGroupAst> parseOptionGroup();
        
        Parser& operator|(const IOption& option);
    };
}
