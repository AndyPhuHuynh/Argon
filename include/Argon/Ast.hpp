# pragma once

#include <memory>
#include <string>
#include <vector>

#include "Error.hpp"

namespace Argon {
    class IOption;
    class Parser;
    class OptionAst;
    class OptionGroupAst;
    class StatementAst;
    class Context;
    class Token;
    
    struct Value {
        std::string value;
        int pos;
    };
    
    class Ast {
    public:
        inline virtual void analyze(ErrorGroup& errorGroup, Context& context) = 0;
    protected:
        inline virtual ~Ast() = default;
    };

    class OptionBaseAst : public Ast {
    public:
        Value flag;
        
        inline ~OptionBaseAst() override = default;
    protected:
        inline explicit OptionBaseAst(const Token& flagToken);
    };
    
    class OptionAst : public OptionBaseAst {
    public:
        Value value;
        
        inline OptionAst(const Token& flagToken, const Token& valueToken);
        inline ~OptionAst() override = default;

        inline void analyze(ErrorGroup& errorGroup, Context& context) override;
    };

    class MultiOptionAst : public OptionBaseAst {
        std::vector<Value> m_values;

    public:
        inline explicit MultiOptionAst(const Token& flagToken);
        inline ~MultiOptionAst() override = default;

        inline void addValue(const Token& value);
        inline void analyze(ErrorGroup& errorGroup, Context& context) override;
    };
    
    class OptionGroupAst : public OptionBaseAst {
    public:
        int endPos = -1;

        inline explicit OptionGroupAst(const Token& flagToken);
        
        inline OptionGroupAst(const OptionGroupAst&) = delete;
        inline OptionGroupAst& operator=(const OptionGroupAst&) = delete;
        
        inline OptionGroupAst(OptionGroupAst&&) noexcept = default;
        inline OptionGroupAst& operator=(OptionGroupAst&&) noexcept = default;
        
        inline ~OptionGroupAst() override = default;
        
        inline void addOption(std::unique_ptr<OptionBaseAst> option);
        inline void analyze(ErrorGroup& errorGroup, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };

    class StatementAst : public Ast {
    public:

        inline StatementAst() = default;
        
        inline StatementAst(const StatementAst&) = delete;
        inline StatementAst& operator=(const StatementAst&) = delete;
        
        inline StatementAst(StatementAst&&) noexcept = default;
        inline StatementAst& operator=(StatementAst&&) noexcept = default;
        
        inline ~StatementAst() override = default;
        
        inline void addOption(std::unique_ptr<OptionBaseAst> option);
        inline void analyze(ErrorGroup& errorGroup, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };
}

#include "Ast.tpp"
