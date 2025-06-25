#ifndef ARGON_AST_INCLUDE
#define ARGON_AST_INCLUDE

#include <memory>
#include <string>
#include <vector>

#include "ParserConfig.hpp"
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
        void checkPositionals(Parser& parser, const Context& context) const;
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
        std::vector<std::unique_ptr<PositionalAst>> m_positionals;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <utility>

#include "Context.hpp"
#include "Option.hpp" // NOLINT (unused include)
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
    IOption *iOption = context.getFlagOption(flag.value);
    if (!iOption) {
        parser.addAnalysisError(std::format("Unknown option: '{}'", flag.value), flag.pos, ErrorType::Analysis_UnknownFlag);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(iOption);
    if (!setValue || !dynamic_cast<IsSingleOption*>(iOption)) {
        parser.addAnalysisError(std::format("Flag '{}' is not an option", flag.value), flag.pos, ErrorType::Analysis_IncorrectOptionType);
        return;
    }

    setValue->setValue(parser.getConfig(), flag.value, value.value);
    if (iOption->hasError()) {
        parser.addAnalysisError(iOption->getError(), value.pos, ErrorType::Analysis_ConversionError);
    }
}

// MultiOptionAst

inline Argon::MultiOptionAst::MultiOptionAst(const Token& flagToken) : OptionBaseAst(flagToken) {}

inline void Argon::MultiOptionAst::addValue(const Token& value) {
    m_values.emplace_back(value.image, value.position);
}

inline void Argon::MultiOptionAst::analyze(Parser& parser, Context &context) {
    IOption *iOption = context.getFlagOption(flag.value);
    if (!iOption) {
        parser.addAnalysisError(std::format("Unknown multi-option: '{}'", flag.value), flag.pos, ErrorType::Analysis_UnknownFlag);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(iOption);
    if (!setValue || !dynamic_cast<IsMultiOption*>(iOption)) {
        parser.addAnalysisError(std::format("Flag '{}' is not a multi-option", flag.value),
            flag.pos, ErrorType::Analysis_IncorrectOptionType);
        return;
    }

    for (const auto&[value, pos] : m_values) {
        setValue->setValue(parser.getConfig(), flag.value, value);
        if (iOption->hasError()) {
            parser.addAnalysisError(iOption->getError(), pos, ErrorType::Analysis_ConversionError);
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
        parser.addAnalysisError(std::format("Unexpected token: '{}'", value.value), value.pos, ErrorType::Analysis_UnexpectedToken);
        return;
    }

    auto *setValue = dynamic_cast<ISetValue*>(opt);
    const auto *iOption = dynamic_cast<IOption*>(opt);
    if (!setValue || !iOption) std::unreachable();

    setValue->setValue(parser.getConfig(), iOption->getInputHint(), value.value);
    if (iOption->hasError()) {
        parser.addAnalysisError(iOption->getError(), value.pos, ErrorType::Analysis_ConversionError);
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
    IOption *iOption = context.getFlagOption(flag.value);
    if (!iOption) {
        parser.removeErrorGroup(flag.pos);
        parser.addAnalysisError(std::format("Unknown option group: '{}'", flag.value), flag.pos, ErrorType::Analysis_UnknownFlag);
        return;
    }

    const auto optionGroup = dynamic_cast<OptionGroup*>(iOption);
    if (!optionGroup) {
        parser.removeErrorGroup(flag.pos);
        parser.addAnalysisError(std::format("Flag '{}' is not an option group", flag.value),
            flag.pos, ErrorType::Analysis_IncorrectOptionType);
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

inline void Argon::StatementAst::checkPositionals(Parser& parser, const Context& context) const {
    const auto policy =
        context.getPositionalPolicy() != PositionalPolicy::None ?
        context.getPositionalPolicy() : parser.getConfig().getPositionalPolicy();
    switch (policy) {
        case PositionalPolicy::None:
            throw std::invalid_argument("Cannot validate PositionPolicy::None");
        case PositionalPolicy::Interleaved:
            break;
        case PositionalPolicy::BeforeFlags: {
            if (m_options.empty() || m_positionals.empty()) break;
            size_t flagIndex = 0;
            for (const auto& positional : m_positionals) {
                while (flagIndex < m_options.size() && m_options[flagIndex]->flag.pos < positional->value.pos) {
                    flagIndex++;
                }
                if (flagIndex > m_options.size()) break;
                if (flagIndex == 0) continue;
                parser.addSyntaxError(std::format(
                    "Found positional value '{}' after flag '{}'. Positional values must occur before all flags.",
                    positional->value.value, m_options[flagIndex - 1]->flag.value),
                    positional->value.pos, ErrorType::Syntax_MisplacedPositional);
            }
            break;
        }
        case PositionalPolicy::AfterFlags: {
            if (m_options.empty() || m_positionals.empty()) break;
            size_t flagIndex = 0;
            for (const auto& positional : m_positionals) {
                while (flagIndex < m_options.size() && m_options[flagIndex]->flag.pos > positional->value.pos) {
                    flagIndex++;
                }
                if (flagIndex > m_options.size()) break;
                if (flagIndex == 0) continue;
                parser.addSyntaxError(std::format(
                    "Found positional value '{}' before flag '{}'. Positional values must occur after all flags.",
                    positional->value.value, m_options[flagIndex - 1]->flag.value),
                    positional->value.pos, ErrorType::Syntax_MisplacedPositional);
                break;
            }
        }
    }
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