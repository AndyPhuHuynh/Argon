#pragma once

#include <memory>
#include <string>

#include "Ast.hpp"
#include "Context.hpp"
#include "Error.hpp"
#include "Scanner.hpp"

namespace Argon {
    class IOption;
    template<typename T>
    class Option;
    
    class Parser {
        Context m_context;
        Scanner m_scanner = Scanner();

        ErrorGroup m_syntaxErrors = ErrorGroup("Syntax Errors", -1, -1);
        ErrorGroup m_analysisErrors = ErrorGroup("Analysis Errors", -1, -1);
    public:
        void addOption(const IOption& option);

        void addError(const std::string& error, int pos);
        void addErrorGroup(const std::string& groupName, int startPos, int endPos);
        void removeErrorGroup(int startPos);
        
        void parseString(const std::string& str);
        StatementAst parseStatement();
        std::unique_ptr<OptionAst> parseOption(Context& context);
        std::unique_ptr<OptionGroupAst> parseOptionGroup(Context& context);
        
        Parser& operator|(const IOption& option);
    };
}
