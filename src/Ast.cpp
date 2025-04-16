#include "Argon/Ast.hpp"

#include "Argon/Option.hpp"

Argon::OptionBaseAst::OptionBaseAst(std::string name) : flag(std::move(name)) {}

// OptionAst

Argon::OptionAst::OptionAst(const std::string& name, std::string value, const int flagPos, const int valuePos)
    : OptionBaseAst(name), value(std::move(value)), flagPos(flagPos), valuePos(valuePos) {}

void Argon::OptionAst::analyze(Parser& parser, const std::vector<IOption*>& options) {
    IOption *iOption = Parser::getOption(options, flag);
    if (!iOption) {
        parser.addError("Option '" + flag + "' not found", flagPos);
        return;
    }
    
    OptionBase *optionBase = dynamic_cast<OptionBase*>(iOption);
    if (!optionBase) {
        parser.addError("Flag '" + flag + "' is not an option", flagPos);
        return;
    }

    optionBase->set_value(flag, value);
    if (iOption->has_error()) {
        parser.addError(iOption->get_error(), valuePos);
    }
}

// OptionGroupAst

Argon::OptionGroupAst::OptionGroupAst(const std::string& name, const int flagPos)
    : OptionBaseAst(name), flagPos(flagPos) {}

void Argon::OptionGroupAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    m_options.push_back(std::move(option)); 
}

void Argon::OptionGroupAst::analyze(Parser& parser, const std::vector<IOption*>& options) {
    parser.addGroupToParseStack(flag);
    
    IOption *iOption = Parser::getOption(options, flag);
    if (!iOption) {
        parser.addError("Option '" + flag + "' not found", flagPos);
        return;
    }
    
    OptionGroup *optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        parser.addError("Flag '" + flag + "' is not an option group", flagPos);
        return;
    }
    
    for (const auto& option : m_options) {
        option->analyze(parser, optionGroup->get_options());
    }

    parser.popGroupParseStack();
}

// StatementAst

void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    m_options.push_back(std::move(option));    
}

void Argon::StatementAst::analyze(Parser& parser, const std::vector<IOption*>& options) {
    for (const auto& option : m_options) {
        option->analyze(parser, options);
    }
}