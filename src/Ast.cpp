#include "Argon/Ast.hpp"

#include "Argon/Context.hpp"
#include "Argon/Option.hpp"

Argon::OptionBaseAst::OptionBaseAst(std::string name) : flag(std::move(name)) {}

// OptionAst

Argon::OptionAst::OptionAst(const std::string& name, std::string value, const int flagPos, const int valuePos)
    : OptionBaseAst(name), value(std::move(value)), flagPos(flagPos), valuePos(valuePos) {}

void Argon::OptionAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag);
    if (!iOption) {
        parser.addError("Unknown option: '" + flag + "'", flagPos);
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
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option)); 
}

void Argon::OptionGroupAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag);
    if (!iOption) {
        parser.removeErrorGroup(flagPos);
        parser.addError("Unknown option group: '" + flag + "'", flagPos);
        return;
    }
    
    OptionGroup *optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        parser.removeErrorGroup(flagPos);
        parser.addError("Flag '" + flag + "' is not an option group", flagPos);
        return;
    }
    
    for (const auto& option : m_options) {
        option->analyze(parser, optionGroup->get_context());
    }
}

// StatementAst

void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));    
}

void Argon::StatementAst::analyze(Parser& parser, Context& context) {
    for (const auto& option : m_options) {
        option->analyze(parser, context);
    }
}