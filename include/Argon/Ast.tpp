#pragma once
#include <utility>

#include "Argon/Ast.hpp"

#include "Context.hpp"
#include "Scanner.hpp"
#include "Option.hpp"
#include "MultiOption.hpp"

Argon::OptionBaseAst::OptionBaseAst(const Token& flagToken) {
    flag = { .value = flagToken.image, .pos = flagToken.position };
}

// OptionAst

Argon::OptionAst::OptionAst(const Token& flagToken, const Token& valueToken)
    : OptionBaseAst(flagToken) {
    value = { .value = valueToken.image, .pos = valueToken.position };
}

void Argon::OptionAst::analyze(ErrorGroup& errorGroup, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.addErrorMessage(std::format("Unknown option: '{}'", flag.value), flag.pos);
        return;
    }

    OptionBase *optionBase = dynamic_cast<OptionBase*>(iOption);
    if (!optionBase || !dynamic_cast<IsSingleOption*>(iOption)) {
        errorGroup.addErrorMessage(std::format("Flag '{}' is not an option", flag.value), flag.pos);
        return;
    }

    optionBase->set_value(flag.value, value.value);
    if (iOption->has_error()) {
        errorGroup.addErrorMessage(iOption->get_error(), value.pos);
    }
}

// MultiOptionAst

Argon::MultiOptionAst::MultiOptionAst(const Token& flagToken) : OptionBaseAst(flagToken) {

}

void Argon::MultiOptionAst::addValue(const Token& value) {
    m_values.emplace_back(value.image, value.position);
}

void Argon::MultiOptionAst::analyze(ErrorGroup& errorGroup, Context &context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.addErrorMessage(std::format("Unknown multi-option: '{}'", flag.value), flag.pos);
        return;
    }

    OptionBase *optionBase = dynamic_cast<OptionBase*>(iOption);
    if (!optionBase || !dynamic_cast<IsMultiOption*>(iOption)) {
        errorGroup.addErrorMessage(std::format("Flag '{}' is not a multi-option", flag.value), flag.pos);
        return;
    }

    for (const auto&[value, pos] : m_values) {
        optionBase->set_value(flag.value, value);
        if (iOption->has_error()) {
            errorGroup.addErrorMessage(iOption->get_error(), pos);
        }
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

void Argon::OptionGroupAst::analyze(ErrorGroup& errorGroup, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.removeErrorGroup(flag.pos);
        errorGroup.addErrorMessage(std::format("Unknown option group: '{}'", flag.value), flag.pos);
        return;
    }

    OptionGroup *optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        errorGroup.removeErrorGroup(flag.pos);
        errorGroup.addErrorMessage(std::format("Flag '{}' is not an option group", flag.value), flag.pos);
        return;
    }

    for (const auto& option : m_options) {
        option->analyze(errorGroup, optionGroup->get_context());
    }
}

// StatementAst

void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));
}

void Argon::StatementAst::analyze(ErrorGroup& errorGroup, Context& context) {
    for (const auto& option : m_options) {
        option->analyze(errorGroup, context);
    }
}