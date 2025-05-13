# pragma once

#include <memory>
#include <string>
#include <vector>

#include "Scanner.hpp"

namespace Argon {
    class IOption;
    class Parser;
    class OptionAst;
    class OptionGroupAst;
    class StatementAst;
    class Context;
    
    struct Value {
        std::string value;
        int pos;
    };
    
    class Ast {
    public:
        virtual void analyze(Parser& parser, Context& context) = 0;
    protected:
        virtual ~Ast() = default;
    };

    class OptionBaseAst : public Ast {
    public:
        Value flag;
        
        ~OptionBaseAst() override = default;
    protected:
        explicit OptionBaseAst(const Token& flagToken);
    };
    
    class OptionAst : public OptionBaseAst {
    public:
        Value value;
        
        OptionAst(const Token& flagToken, const Token& valueToken);
        ~OptionAst() override = default;

        void analyze(Parser& parser, Context& context) override;
    };

    class MultiOptionAst : public OptionBaseAst {
        std::vector<Value> m_values;

    public:
        explicit MultiOptionAst(const Token& flagToken);
        ~MultiOptionAst() override = default;

        void addValue(const Token& value);
        void analyze(Parser& parser, Context& context) override;
    };
    
    class OptionGroupAst : public OptionBaseAst {
    public:
        int endPos = -1;

        explicit OptionGroupAst(const Token& flagToken);
        
        OptionGroupAst(const OptionGroupAst&) = delete;
        OptionGroupAst& operator=(const OptionGroupAst&) = delete;
        
        OptionGroupAst(OptionGroupAst&&) noexcept = default;
        OptionGroupAst& operator=(OptionGroupAst&&) noexcept = default;
        
        ~OptionGroupAst() override = default;
        
        void addOption(std::unique_ptr<OptionBaseAst> option);
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };

    class StatementAst : public Ast {
    public:

        StatementAst() = default;
        
        StatementAst(const StatementAst&) = delete;
        StatementAst& operator=(const StatementAst&) = delete;
        
        StatementAst(StatementAst&&) noexcept = default;
        StatementAst& operator=(StatementAst&&) noexcept = default;
        
        ~StatementAst() override = default;
        
        void addOption(std::unique_ptr<OptionBaseAst> option);
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };
}
