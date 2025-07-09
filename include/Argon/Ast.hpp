#ifndef ARGON_AST_INCLUDE
#define ARGON_AST_INCLUDE

#include <memory>
#include <string>
#include <vector>

#include "Config/ContextConfig.hpp"
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
        [[nodiscard]] virtual auto getGroupPath() const -> std::string;

        auto setParent(OptionGroupAst *parent) -> void;

        virtual auto analyze(Parser& parser, Context& context) -> void = 0;
    protected:
        OptionGroupAst *m_parent = nullptr;

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

    class PositionalAst : public Ast {
    public:
        Value value;

        explicit PositionalAst(const Token& flagToken);

        void analyze(Parser& parser, Context& context) override;

        void analyze(Parser& parser, Context& context, size_t position);
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

        [[nodiscard]] auto getGroupPath() const -> std::string override;
        void addOption(std::unique_ptr<OptionBaseAst> option);
        void addOption(std::unique_ptr<PositionalAst> option);
        void checkPositionals(Parser& parser, Context& context) const;
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
        void checkPositionals(Parser& parser, Context& context) const;
        void analyze(Parser& parser, Context& context) override;
    private:
        std::vector<std::unique_ptr<OptionBaseAst>> m_options;
        std::vector<std::unique_ptr<PositionalAst>> m_positionals;
    };
}

//---------------------------------------------------Free Functions-----------------------------------------------------

#include <utility>

#include "Context.hpp"
#include "Options/Option.hpp"
#include "Parser.hpp" // NOLINT (unused include)
#include "Options/MultiOption.hpp"

namespace Argon {

inline auto checkPositionals(const Ast& thisAst,
                             Parser& parser,
                             const Context& context,
                             const std::vector<std::unique_ptr<OptionBaseAst>>& options,
                             const std::vector<std::unique_ptr<PositionalAst>>& positionals) {
    const std::string contextPath = thisAst.getGroupPath();
    const auto policy =
        context.getDefaultPositionalPolicy() != PositionalPolicy::UseDefault ?
        context.getDefaultPositionalPolicy() : parser.getConfig().getDefaultPositionalPolicy();
    switch (policy) {
        case PositionalPolicy::UseDefault:
            throw std::invalid_argument("Cannot validate PositionPolicy::None");
        case PositionalPolicy::Interleaved:
            break;
        case PositionalPolicy::BeforeFlags: {
            if (options.empty() || positionals.empty()) break;
            size_t flagIndex = 0;
            for (const auto& positional : positionals) {
                while (flagIndex < options.size() && options[flagIndex]->flag.pos < positional->value.pos) {
                    flagIndex++;
                }
                if (flagIndex > options.size()) break;
                if (flagIndex == 0) continue;
                if (contextPath.empty()) {
                    parser.addSyntaxError(std::format(
                        "Found positional value '{}' after flag '{}'. Positional values must occur before all flags at top level.",
                        positional->value.value, options[flagIndex - 1]->flag.value),
                        positional->value.pos, ErrorType::Syntax_MisplacedPositional);
                } else {
                    parser.addSyntaxError(std::format(
                        "Found positional value '{}' after flag '{}' inside group '{}'. "
                        "Positional values must occur before all flags inside group '{}'.",
                        positional->value.value, options[flagIndex - 1]->flag.value, contextPath, contextPath),
                        positional->value.pos, ErrorType::Syntax_MisplacedPositional);
                }
            }
            break;
        }
        case PositionalPolicy::AfterFlags: {
            if (options.empty() || positionals.empty()) break;
            size_t flagIndex = 0;
            for (const auto& positional : positionals) {
                // Find option directly after each positional
                while (flagIndex < options.size() && options[flagIndex]->flag.pos < positional->value.pos) {
                    flagIndex++;
                }
                if (flagIndex >= options.size()) break;
                if (contextPath.empty()) {
                    parser.addSyntaxError(std::format(
                        "Found positional value '{}' before flag '{}'. Positional values must occur after all flags at top level.",
                        positional->value.value, options[flagIndex]->flag.value),
                        positional->value.pos, ErrorType::Syntax_MisplacedPositional);
                } else {
                    parser.addSyntaxError(std::format(
                        "Found positional value '{}' before flag '{}' inside group '{}'. "
                        "Positional values must occur after all flags inside group '{}'.",
                        positional->value.value, options[flagIndex]->flag.value, contextPath, contextPath),
                        positional->value.pos, ErrorType::Syntax_MisplacedPositional);
                }
            }
            break;
        }
    }
}

}

// --------------------------------------------- Implementations -------------------------------------------------------

inline std::string Argon::Ast::getGroupPath() const {
    return "";
}

inline auto Argon::Ast::setParent(OptionGroupAst *parent) -> void {
    m_parent = parent;
}

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

inline void Argon::PositionalAst::analyze(Parser&, Context&) {}

inline void Argon::PositionalAst::analyze(Parser& parser, Context& context, const size_t position) {
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

inline std::string Argon::OptionGroupAst::getGroupPath() const {
    std::vector<const std::string*> names;

    const auto *current = this;
    while (current != nullptr) {
        names.push_back(&current->flag.value);
        current = current->m_parent;
    }

    std::ranges::reverse(names);
    return std::accumulate(std::next(names.begin()), names.end(), *(names[0]),
        [] (const std::string& name1, const std::string *name2) {
            return name1 + " > " + *name2;
    });
}

inline void Argon::OptionGroupAst::addOption(std::unique_ptr<OptionBaseAst> option) {
    if (option == nullptr) {
        return;
    }
    m_options.push_back(std::move(option));
    m_options.back()->setParent(this);
}

inline void Argon::OptionGroupAst::addOption(std::unique_ptr<PositionalAst> option) {
    if (option == nullptr) {
        return;
    }
    m_positionals.push_back(std::move(option));
    m_positionals.back()->setParent(this);
}

inline void Argon::OptionGroupAst::checkPositionals(Parser& parser, Context& context) const { //NOLINT (misc-recursion)
    Argon::checkPositionals(*this, parser, context, m_options, m_positionals);
    for (const auto& optionAst : m_options) {
        if (const auto groupAst = dynamic_cast<OptionGroupAst*>(optionAst.get())) {
            const auto iOption = context.getFlagOption(groupAst->flag.value);
            if (const auto groupPtr = dynamic_cast<OptionGroup*>(iOption)) {
                groupAst->checkPositionals(parser, groupPtr->getContext());
            }
        }
    }
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

inline void Argon::StatementAst::checkPositionals(Parser& parser, Context& context) const {
    Argon::checkPositionals(*this, parser, context, m_options, m_positionals);
    for (const auto& optionAst : m_options) {
        if (const auto groupAst = dynamic_cast<OptionGroupAst*>(optionAst.get())) {
            const auto iOption = context.getFlagOption(groupAst->flag.value);
            if (const auto groupPtr = dynamic_cast<OptionGroup*>(iOption)) {
                groupAst->checkPositionals(parser, groupPtr->getContext());
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