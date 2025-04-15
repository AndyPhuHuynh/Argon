# pragma once

#include <memory>
#include <string>
#include <vector>

namespace Argon {
    class IOption;
    class Parser;
    class OptionAst;
    class OptionGroupAst;
    class StatementAst;

    class Ast {
    public:
        virtual void analyze(Parser& parser, const std::vector<IOption*>& options) = 0;
    protected:
        virtual ~Ast() = default;
    };

    class OptionBaseAst : public Ast {
    public:
        std::string flag;
        ~OptionBaseAst() override = default;
    protected:
        OptionBaseAst(std::string name);
    };
    
    class OptionAst : public OptionBaseAst {
    public:
        std::string value;
        
        OptionAst(const std::string& name, std::string value);
        ~OptionAst() override = default;

        void analyze(Parser& parser, const std::vector<IOption*>& options) override;
    };

    class OptionGroupAst : public OptionBaseAst {
    public:
        OptionGroupAst(const std::string& name);
        
        OptionGroupAst(const OptionGroupAst&) = delete;
        OptionGroupAst& operator=(const OptionGroupAst&) = delete;
        
        OptionGroupAst(OptionGroupAst&&) noexcept = default;
        OptionGroupAst& operator=(OptionGroupAst&&) noexcept = default;
        
        ~OptionGroupAst() override = default;
        
        void addOption(std::unique_ptr<OptionBaseAst> option);
        void analyze(Parser& parser, const std::vector<IOption*>& options) override;
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
        void analyze(Parser& parser, const std::vector<IOption*>& options) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };
}
