#include <utility>

#include "Argon/Ast.hpp"

#include "Argon/Context.hpp"
#include "Argon/Option.hpp"

Argon::OptionBaseAst::OptionBaseAst(const Token& flagToken) {
    flag = { .value = flagToken.image, .pos = flagToken.position };
}

// OptionAst

Argon::OptionAst::OptionAst(const Token& flagToken, const Token& valueToken)
    : OptionBaseAst(flagToken) {
    value = { .value = valueToken.image, .pos = valueToken.position };
}

void Argon::OptionAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        parser.addError(std::format("Unknown option: '{}'", flag.value), flag.pos);
        return;
    }
    
    OptionBase *optionBase = dynamic_cast<OptionBase*>(iOption);
    if (!optionBase) {
        parser.addError(std::format("Flag '{}' is not an option", flag.value), flag.pos);
        return;
    }

    optionBase->set_value(flag.value, value.value);
    if (iOption->has_error()) {
        parser.addError(iOption->get_error(), value.pos);
    }
}

// OptionGroupAst

Argon::OptionGroupAst::OptionGroupAst(const Token& flagToken)
    : OptionBaseAst(flagToken) {}

void Argon::OptionGroupAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option)); 
}

void Argon::OptionGroupAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        parser.removeErrorGroup(flag.pos);
        parser.addError(std::format("Unknown option group: '{}'", flag.value), flag.pos);
        return;
    }
    
    OptionGroup *optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        parser.removeErrorGroup(flag.pos);
        parser.addError(std::format("Flag '{}' is not an option group", flag.value), flag.pos);
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