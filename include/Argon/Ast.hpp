# pragma once

#include <memory>
#include <string>
#include <vector>

#include "Scanner.hpp"
#include "Error.hpp"

namespace Argon {
    class Context;

    struct Value {
        std::string value;
        int pos = 0;
    };

    class Ast {
    public:
        virtual void analyze(ErrorGroup& errorGroup, Context& context) = 0;
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

        void analyze(ErrorGroup& errorGroup, Context& context) override;
    };

    class MultiOptionAst : public OptionBaseAst {
        std::vector<Value> m_values;

    public:
        explicit MultiOptionAst(const Token& flagToken);
        ~MultiOptionAst() override = default;

        void addValue(const Token& value);
        void analyze(ErrorGroup& errorGroup, Context& context) override;
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
        void analyze(ErrorGroup& errorGroup, Context& context) override;
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
        void analyze(ErrorGroup& errorGroup, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <utility>

#include "Context.hpp"
#include "Option.hpp"
#include "MultiOption.hpp"

inline Argon::OptionBaseAst::OptionBaseAst(const Token& flagToken) {
    flag = { .value = flagToken.image, .pos = flagToken.position };
}

// OptionAst

inline Argon::OptionAst::OptionAst(const Token& flagToken, const Token& valueToken)
    : OptionBaseAst(flagToken) {
    value = { .value = valueToken.image, .pos = valueToken.position };
}

inline void Argon::OptionAst::analyze(ErrorGroup& errorGroup, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.addErrorMessage(std::format("Unknown option: '{}'", flag.value), flag.pos);
        return;
    }

    auto *optionBase = dynamic_cast<OptionBase*>(iOption);
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

inline Argon::MultiOptionAst::MultiOptionAst(const Token& flagToken) : OptionBaseAst(flagToken) {

}

inline void Argon::MultiOptionAst::addValue(const Token& value) {
    m_values.emplace_back(value.image, value.position);
}

inline void Argon::MultiOptionAst::analyze(ErrorGroup& errorGroup, Context &context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.addErrorMessage(std::format("Unknown multi-option: '{}'", flag.value), flag.pos);
        return;
    }

    auto *optionBase = dynamic_cast<OptionBase*>(iOption);
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

inline Argon::OptionGroupAst::OptionGroupAst(const Token& flagToken)
    : OptionBaseAst(flagToken) {}

inline void Argon::OptionGroupAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));
}

inline void Argon::OptionGroupAst::analyze(ErrorGroup& errorGroup, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        errorGroup.removeErrorGroup(flag.pos);
        errorGroup.addErrorMessage(std::format("Unknown option group: '{}'", flag.value), flag.pos);
        return;
    }

    auto optionGroup = dynamic_cast<OptionGroup*>(iOption);
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

inline void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));
}

inline void Argon::StatementAst::analyze(ErrorGroup& errorGroup, Context& context) {
    for (const auto& option : m_options) {
        option->analyze(errorGroup, context);
    }
}