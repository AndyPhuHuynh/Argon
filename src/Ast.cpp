#include "Argon/Ast.hpp"

#include "Argon/Option.hpp"

Argon::OptionBaseAst::OptionBaseAst(std::string name) : flag(std::move(name)) {}

// OptionAst

Argon::OptionAst::OptionAst(const std::string& name, std::string value) : OptionBaseAst(name), value(std::move(value)) {}

void Argon::OptionAst::analyze(const std::vector<IOption*>& options) {
    IOption *iOption = Parser::getOption(options, flag);
    if (!iOption) {
        std::cerr << "Error: Option '" << flag << "' not found!\n";
    }
    
    OptionBase *optionBase = dynamic_cast<OptionBase*>(iOption);
    if (!optionBase) {
        std::cerr << "Error: Option '" << flag << "' is not an option!\n";
    }

    optionBase->set_value(flag, value);
}

// OptionGroupAst

Argon::OptionGroupAst::OptionGroupAst(const std::string& name) : OptionBaseAst(name) {}

void Argon::OptionGroupAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    m_options.push_back(std::move(option)); 
}

void Argon::OptionGroupAst::analyze(const std::vector<IOption*>& options) {
    IOption *iOption = Parser::getOption(options, flag);
    if (!iOption) {
        std::cerr << "Error: Option '" << flag << "' not found!\n";
    }
    
    OptionGroup *optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        std::cerr << "Error: Option '" << flag << "' is not an option group!\n";
    }
    
    for (const auto& option : m_options) {
        option->analyze(optionGroup->get_options());
    }
}

// StatementAst

void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    m_options.push_back(std::move(option));    
}

void Argon::StatementAst::analyze(const std::vector<IOption*>& options) {
    for (const auto& option : m_options) {
        option->analyze(options);
    }
}