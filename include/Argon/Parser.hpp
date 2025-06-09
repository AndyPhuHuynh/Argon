#ifndef ARGON_PARSER_INCLUDE
#define ARGON_PARSER_INCLUDE

#include <memory>
#include <string>

#include "Ast.hpp"
#include "Attributes.hpp"
#include "Context.hpp"
#include "Error.hpp"
#include "Option.hpp"
#include "MultiOption.hpp"
#include "Scanner.hpp"
#include "Traits.hpp"

namespace Argon {
    class Parser {
        Context m_context;
        Scanner m_scanner;

        std::vector<std::string> m_contextValidationErrors;
        ErrorGroup m_syntaxErrors = ErrorGroup("Syntax Errors", -1, -1);
        ErrorGroup m_analysisErrors = ErrorGroup("Analysis Errors", -1, -1);
        std::vector<std::string> m_constraintErrors;

        std::vector<Token> m_brackets;
        bool m_mismatchedRBRACK = false;

        Constraints m_constraints;

        std::string m_shortPrefix = "-";
        std::string m_longPrefix  = "--";
    public:
        Parser() = default;

        template<typename T> requires DerivesFrom<T, IOption>
        explicit Parser(T&& option);

        template<typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto addError(const std::string& error, int pos) -> void;

        auto addErrorGroup(const std::string& groupName, int startPos, int endPos) -> void;

        auto removeErrorGroup(int startPos) -> void;

        [[nodiscard]] bool hasErrors() const;

        auto printErrors(PrintMode analysisPrintMode, TextMode analysisTextMode = TextMode::Ascii) const -> void;

        auto parse(int argc, const char **argv);

        auto parse(const std::string& str) -> bool;

        template<typename T> requires DerivesFrom<T, IOption>
        auto operator|(T&& option) -> Parser&;

        template<typename ValueType>
        auto getValue(const std::string& flag) -> const ValueType&;

        template<typename ValueType>
        auto getValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(const std::string& flag) -> const Container&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        auto constraints() -> Constraints&;

    private:
        auto reset() -> void;

        auto parseStatement() -> StatementAst;

        auto parseOptionBundle(Context& context) -> std::unique_ptr<OptionBaseAst>;

        auto parseSingleOption(Context& context, const Token& flag) -> std::unique_ptr<OptionAst>;

        auto parseMultiOption(const Context& context, const Token& flag) -> std::unique_ptr<MultiOptionAst>;

        auto parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) -> void;

        auto parseOptionGroup(Context& context, const Token& flag) -> std::unique_ptr<OptionGroupAst>;

        auto getNextValidFlag(const Context& context, bool printErrors = true) -> std::optional<Token>;

        auto skipToNextValidFlag(const Context& context) -> void;

        auto getNextToken() -> Token;

        auto skipScope() -> void;

        auto validateConstraints() -> void;

        auto applyPrefixes() -> void;
    };

    template<typename Left, typename Right> requires DerivesFrom<Left, IOption> && DerivesFrom<Right, IOption>
    auto operator|(Left&& left, Right&& right) -> Parser;
}

// --------------------------------------------- Implementations -------------------------------------------------------

namespace Argon {
template<typename T> requires DerivesFrom<T, IOption>
Parser::Parser(T&& option) {
    addOption(std::forward<T>(option));
}


template<typename T> requires DerivesFrom<T, IOption>
auto Parser::addOption(T&& option) -> void {
    m_context.addOption(std::forward<T>(option));
}

inline auto Parser::addError(const std::string& error, const int pos) -> void {
    m_analysisErrors.addErrorMessage(error, pos);
}

inline auto Parser::addErrorGroup(const std::string& groupName, const int startPos, const int endPos) -> void {
    m_analysisErrors.addErrorGroup(groupName, startPos, endPos);
}

inline auto Parser::removeErrorGroup(const int startPos) -> void {
    m_analysisErrors.removeErrorGroup(startPos);
}

inline auto Parser::hasErrors() const -> bool {
    return !m_contextValidationErrors.empty() || m_syntaxErrors.hasErrors() ||
            m_analysisErrors.hasErrors() || !m_constraintErrors.empty();
}

inline auto Parser::printErrors(const PrintMode analysisPrintMode,
                                const TextMode analysisTextMode) const -> void {
    if (!m_contextValidationErrors.empty()) {
        std::cout << "Parser is in an invalid state:\n";
        for (const auto& err : m_contextValidationErrors) {
            std::cout << " - " << err << "\n";
        }
        return;
    }
    if (m_syntaxErrors.hasErrors()) {
        m_syntaxErrors.printErrorsFlatMode();
        return;
    }
    if (m_analysisErrors.hasErrors()) {
        switch (analysisPrintMode) {
            case PrintMode::Flat:
                m_analysisErrors.printErrorsFlatMode();
                return;
            case PrintMode::Tree:
                m_analysisErrors.printErrorsTreeMode(analysisTextMode);
        }
        return;
    }
    if (!m_constraintErrors.empty()) {
        for (const auto& err : m_constraintErrors) {
            std::cout << err << "\n";
        }
    }
}

inline auto Parser::parse(const int argc, const char **argv) {
    std::string input;
    for (int i = 1; i < argc; i++) {
        input += argv[i];
        input += " ";
    }
    parse(input);
}

inline auto Parser::parse(const std::string& str) -> bool {
    m_context.applyPrefixes(m_shortPrefix, m_longPrefix);

    m_context.validate(m_contextValidationErrors);
    if (!m_contextValidationErrors.empty()) {
        return false;
    }

    reset();
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(m_analysisErrors, m_context);
    validateConstraints();
    return !hasErrors();
}

inline auto Parser::parseStatement() -> StatementAst {
    StatementAst statement;
    while (!m_scanner.seeTokenKind(TokenKind::END)) {
        // Handle rbrack that gets leftover after SkipScope
        if (m_scanner.seeTokenKind(TokenKind::RBRACK)) {
            getNextToken();
            continue;
        }
        statement.addOption(parseOptionBundle(m_context));
    }
    return statement;
}

inline auto Parser::parseOptionBundle(Context& context) -> std::unique_ptr<OptionBaseAst> { // NOLINT(misc-no-recursion)
    const auto opt = getNextValidFlag(context);
    if (!opt.has_value()) { return nullptr; }
    const auto& flagToken = opt.value();

    IOption *iOption = context.getOption(flagToken.image);
    if (dynamic_cast<IsSingleOption *>(iOption)) {
        return parseSingleOption(context, flagToken);
    }
    if (dynamic_cast<IsMultiOption *>(iOption)) {
        return parseMultiOption(context, flagToken);
    }
    if (dynamic_cast<OptionGroup *>(iOption)) {
        return parseOptionGroup(context, flagToken);
    }

    return nullptr;
}

inline auto Parser::parseSingleOption(Context& context, const Token& flag) -> std::unique_ptr<OptionAst> {
    Token value = m_scanner.peekToken();

    // Boolean flag with no explicit value
    const bool nextTokenIsAnotherFlag = value.kind == TokenKind::IDENTIFIER && context.containsLocalFlag(value.image);
    if (context.getOptionDynamic<Option<bool>>(flag.image) &&
        (!value.isOneOf({TokenKind::IDENTIFIER, TokenKind::EQUALS}) || nextTokenIsAnotherFlag)) {
        return std::make_unique<OptionAst>(flag, Token(TokenKind::IDENTIFIER, "true", flag.position));
    }

    // Get optional equal sign
    if (value.kind == TokenKind::EQUALS) {
        getNextToken();
        value = m_scanner.peekToken();
    }

    // Get value
    if (value.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(
            std::format("Expected flag value, got '{}' at position {}", value.image, value.position), value.position);
        skipToNextValidFlag(context);
        return nullptr;
    }

    // If value matches a flag (no value supplied)
    if (context.containsLocalFlag(value.image)) {
        if (&context == &m_context) {
            m_syntaxErrors.addErrorMessage(
                std::format("No value provided for flag '{}' at position {}", flag.image, flag.position),
                value.position
            );
        } else {
            m_syntaxErrors.addErrorMessage(
                std::format("No value provided for flag '{}' inside group '{}' at position {}", flag.image,
                            context.getPath(), flag.position),
                value.position
            );
        }
        getNextValidFlag(context, false);
        if (m_scanner.peekToken().kind != TokenKind::END) {
            m_scanner.rewind(1);
        }
        return nullptr;
    }

    getNextToken();
    return std::make_unique<OptionAst>(flag, value);
}

inline auto Parser::parseMultiOption(const Context& context,
                                     const Token& flag) -> std::unique_ptr<MultiOptionAst> {
    auto multiOptionAst = std::make_unique<MultiOptionAst>(flag);

    while (true) {
        Token nextToken = m_scanner.peekToken();

        const bool endOfMultiOption = nextToken.kind != TokenKind::IDENTIFIER || context.containsLocalFlag(
                                          nextToken.image);
        if (endOfMultiOption) {
            return multiOptionAst;
        }

        multiOptionAst->addValue(nextToken);
        getNextToken();
    }
}

inline auto Parser::parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) -> void { // NOLINT(misc-no-recursion)
    while (true) {
        const Token nextToken = m_scanner.peekToken();

        if (nextToken.kind == TokenKind::RBRACK) {
            getNextToken();
            optionGroupAst.endPos = nextToken.position;
            m_analysisErrors.addErrorGroup(optionGroupAst.flag.value, optionGroupAst.flag.pos, optionGroupAst.endPos);
            return;
        }

        if (nextToken.kind == TokenKind::END) {
            getNextToken();
            optionGroupAst.endPos = nextToken.position;
            m_analysisErrors.addErrorGroup(optionGroupAst.flag.value, optionGroupAst.flag.pos, optionGroupAst.endPos);
            m_syntaxErrors.addErrorMessage(
                std::format("No matching ']' found for group '{}'", optionGroupAst.flag.value), nextToken.position);
            return;
        }

        optionGroupAst.addOption(parseOptionBundle(nextContext));
    }
}

inline auto Parser::parseOptionGroup(Context& context, const Token& flag) -> std::unique_ptr<OptionGroupAst> { // NOLINT(misc-no-recursion)
    const Token lbrack = m_scanner.peekToken();
    if (lbrack.kind != TokenKind::LBRACK) {
        m_syntaxErrors.addErrorMessage(
            std::format("Expected '[', got '{}' at position {}", lbrack.image, lbrack.position), lbrack.position);
        skipToNextValidFlag(context);
        return nullptr;
    }
    getNextToken();

    const auto optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    auto& nextContext = optionGroup->getContext();
    nextContext.setName(flag.image);

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);
    parseGroupContents(*optionGroupAst, nextContext);
    return optionGroupAst;
}

inline auto Parser::getNextValidFlag(const Context& context, const bool printErrors) -> std::optional<Token> {
    Token flag = m_scanner.peekToken();

    const bool isIdentifier = flag.kind == TokenKind::IDENTIFIER;
    const bool inContext = context.containsLocalFlag(flag.image);

    if (isIdentifier && inContext) {
        getNextToken();
        return flag;
    }

    if (printErrors && !isIdentifier) {
        m_syntaxErrors.addErrorMessage(
            std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position),
            flag.position
        );
    } else if (printErrors) {
        if (&context == &m_context) {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}'  at position {}", flag.image, flag.position),
                flag.position
            );
        } else {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}' inside group '{}' at position {}", flag.image, context.getPath(),
                            flag.position),
                flag.position
            );
        }
    }

    if (flag.kind == TokenKind::LBRACK) {
        skipScope();
    }

    while (true) {
        Token token = m_scanner.peekToken();
        if (token.kind == TokenKind::LBRACK) {
            skipScope();
            continue;
        } else if (m_mismatchedRBRACK) {
            getNextToken();
            continue;
        } else if (token.kind == TokenKind::RBRACK || token.kind == TokenKind::END) {
            // Escape this scope, leave RBRACK scanning to the function above
            return std::nullopt;
        } else if (token.kind == TokenKind::IDENTIFIER && context.containsLocalFlag(token.image)) {
            getNextToken();
            return token;
        }
        getNextToken();
    }
}

inline auto Parser::skipToNextValidFlag(const Context& context) -> void {
    getNextValidFlag(context, false);
    if (m_scanner.peekToken().kind != TokenKind::END) {
        m_scanner.rewind(1);
    }
}

template <typename T> requires DerivesFrom<T, IOption>
auto Parser::operator|(T&& option) -> Parser& {
    addOption(std::forward<T>(option));
    return *this;
}

template<typename ValueType>
auto Parser::getValue(const std::string& flag) -> const ValueType& {
    return m_context.getValue<ValueType>(FlagPath(flag));
}

template<typename ValueType>
auto Parser::getValue(const FlagPath& flagPath) -> const ValueType& {
    return m_context.getValue<ValueType>(flagPath);
}

template<typename Container>
auto Parser::getMultiValue(const std::string& flag) -> const Container& {
    return m_context.getMultiValue<Container>(FlagPath(flag));
}

template<typename Container>
auto Parser::getMultiValue(const FlagPath& flagPath) -> const Container& {
    return m_context.getMultiValue<Container>(flagPath);
}

inline auto Parser::constraints() -> Constraints& {
    return m_constraints;
}

inline auto Parser::reset() -> void {
    m_contextValidationErrors.clear();
    m_syntaxErrors.clear();
    m_analysisErrors.clear();
    m_constraintErrors.clear();
    m_brackets.clear();
}

inline auto Parser::getNextToken() -> Token {
    m_mismatchedRBRACK = false;
    const Token nextToken = m_scanner.getNextToken();
    if (nextToken.kind == TokenKind::LBRACK) {
        m_brackets.push_back(nextToken);
    } else if (nextToken.kind == TokenKind::RBRACK) {
        if (m_brackets.empty()) {
            m_mismatchedRBRACK = true;
            m_syntaxErrors.addErrorMessage(
                std::format("No matching '[' found for ']' at position {}", nextToken.position),
                nextToken.position
            );
        } else {
            m_brackets.pop_back();
        }
    }
    return nextToken;
}

inline auto Parser::skipScope() -> void {
    if (m_scanner.peekToken().kind != TokenKind::LBRACK) return;
    std::vector<Token> brackets;
    while (true) {
        const Token token = getNextToken();
        if (token.kind == TokenKind::LBRACK) {
            brackets.push_back(token);
        } else if (token.kind == TokenKind::RBRACK) {
            if (brackets.empty()) {
                m_syntaxErrors.addErrorMessage(
                    std::format("Unmatched ']' found at position {}", token.position), token.position);
                return;
            }
            brackets.pop_back();
        }

        if (token.kind == TokenKind::END && !brackets.empty()) {
            for (const auto& bracket: brackets) {
                m_syntaxErrors.addErrorMessage(
                    std::format("Unmatched '[' found at position {}", bracket.position), bracket.position);
            }
            return;
        }

        if (brackets.empty()) {
            return;
        }
    }
}

inline auto Parser::validateConstraints() -> void {
    m_constraints.validate(m_context, m_constraintErrors);
}

inline auto Parser::applyPrefixes() -> void {

}

template<typename Left, typename Right> requires
    Argon::DerivesFrom<Left, IOption> && Argon::DerivesFrom<Right, IOption>
auto operator|(Left&& left, Right&& right) -> Parser {
    Parser parser;
    parser.addOption(std::forward<Left>(left));
    parser.addOption(std::forward<Right>(right));
    return parser;
}
} // End namespace Argon
#endif // ARGON_PARSER_INCLUDE