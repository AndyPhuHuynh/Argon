#ifndef ARGON_AST_INCLUDE
#define ARGON_AST_INCLUDE

#include <memory>
#include <string>
#include <vector>

#include "Scanner.hpp"

namespace Argon {
    class Context;
    class Parser;

    struct Value {
        std::string value;
        int pos = 0;
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

    class PositionalAst {
    public:
        Value value;

        explicit PositionalAst(const Token& flagToken);

        void analyze(Parser& parser, const Context& context, size_t position);
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
        void addOption(std::unique_ptr<PositionalAst> option);
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
        std::vector<std::unique_ptr<PositionalAst>> m_positionals;
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
        void addOption(std::unique_ptr<PositionalAst> option);
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
        std::vector<std::unique_ptr<PositionalAst>> m_positionals;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <utility>

#include "Context.hpp"
#include "Option.hpp"
#include "Parser.hpp" // NOLINT (unused include)
#include "MultiOption.hpp"

inline Argon::OptionBaseAst::OptionBaseAst(const Token& flagToken) {
    flag = { .value = flagToken.image, .pos = flagToken.position };
}

// OptionAst

inline Argon::OptionAst::OptionAst(const Token& flagToken, const Token& valueToken)
    : OptionBaseAst(flagToken) {
    value = { .value = valueToken.image, .pos = valueToken.position };
}

inline void Argon::OptionAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        parser.addError(std::format("Unknown option: '{}'", flag.value), flag.pos);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(iOption);
    if (!setValue || !dynamic_cast<IsSingleOption*>(iOption)) {
        parser.addError(std::format("Flag '{}' is not an option", flag.value), flag.pos);
        return;
    }

    setValue->setValue(parser.getDefaultConversions(), flag.value, value.value);
    if (iOption->hasError()) {
        parser.addError(iOption->getError(), value.pos);
    }
}

// MultiOptionAst

inline Argon::MultiOptionAst::MultiOptionAst(const Token& flagToken) : OptionBaseAst(flagToken) {}

inline void Argon::MultiOptionAst::addValue(const Token& value) {
    m_values.emplace_back(value.image, value.position);
}

inline void Argon::MultiOptionAst::analyze(Parser& parser, Context &context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        parser.addError(std::format("Unknown multi-option: '{}'", flag.value), flag.pos);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(iOption);
    if (!setValue || !dynamic_cast<IsMultiOption*>(iOption)) {
        parser.addError(std::format("Flag '{}' is not a multi-option", flag.value), flag.pos);
        return;
    }

    for (const auto&[value, pos] : m_values) {
        setValue->setValue(parser.getDefaultConversions(), flag.value, value);
        if (iOption->hasError()) {
            parser.addError(iOption->getError(), pos);
        }
    }
}

// PositionalAst

inline Argon::PositionalAst::PositionalAst(const Token& flagToken) {
    value = { .value = flagToken.image, .pos = flagToken.position };
}

inline void Argon::PositionalAst::analyze(Parser& parser, const Context& context, const size_t position) {
    IsPositional *opt = context.getPositional(position);
    if (!opt) {
        parser.addError(std::format("Unknown input: '{}'", value.value), value.pos);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(opt);
    const auto *iOption = dynamic_cast<IOption*>(opt);
    if (!setValue || !iOption) std::unreachable();

    setValue->setValue(parser.getDefaultConversions(), iOption->getInputHint(), value.value);
    if (iOption->hasError()) {
        parser.addError(iOption->getError(), value.pos);
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

inline void Argon::OptionGroupAst::addOption(std::unique_ptr<PositionalAst> option) {
    if (option == nullptr) {
        return;
    }
    m_positionals.push_back(std::move(option));
}

inline void Argon::OptionGroupAst::analyze(Parser& parser, Context& context) {
    IOption *iOption = context.getOption(flag.value);
    if (!iOption) {
        parser.removeErrorGroup(flag.pos);
        parser.addError(std::format("Unknown option group: '{}'", flag.value), flag.pos);
        return;
    }

    const auto optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        parser.removeErrorGroup(flag.pos);
        parser.addError(std::format("Flag '{}' is not an option group", flag.value), flag.pos);
        return;
    }

    auto& nextContext = optionGroup->getContext();
    for (const auto& option : m_options) {
        option->analyze(parser, nextContext);
    }

    for (size_t i = 0; i < m_positionals.size(); i++) {
        m_positionals[i]->analyze(parser, nextContext, i);
    }
}

// StatementAst

inline void Argon::StatementAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));
}

inline void Argon::StatementAst::addOption(std::unique_ptr<PositionalAst> option) {
    if (option == nullptr) {
        return;
    }
    m_positionals.push_back(std::move(option));
}

inline void Argon::StatementAst::analyze(Parser& parser, Context& context) {
    for (const auto& option : m_options) {
        option->analyze(parser, context);
    }

    for (size_t i = 0; i < m_positionals.size(); i++) {
        m_positionals[i]->analyze(parser, context, i);
    }
}

#endif // ARGON_AST_INCLUDE