#ifndef ARGON_PARSER_INCLUDE
#define ARGON_PARSER_INCLUDE

#include <memory>
#include <string>
#include <variant>

#include "Error.hpp"
#include "Flag.hpp"
#include "ParserConfig.hpp"
#include "Scanner.hpp"
#include "Traits.hpp"

namespace Argon {
    class Ast;
    class StatementAst;
    class OptionAst;
    class MultiOptionAst;
    class OptionGroupAst;
    class OptionBaseAst;
    class PositionalAst;

    class IOption;
    class Context;
    class Constraints;

    class Parser {
        std::unique_ptr<Context> m_context = std::make_unique<Context>();
        Scanner m_scanner;

        std::vector<std::string> m_contextValidationErrors;
        ErrorGroup m_syntaxErrors = ErrorGroup("Syntax Errors", -1, -1);
        ErrorGroup m_analysisErrors = ErrorGroup("Analysis Errors", -1, -1);
        std::vector<std::string> m_constraintErrors;

        std::vector<Token> m_brackets;
        std::vector<Token> m_poppedBrackets;
        bool m_mismatchedRBRACK = false;

        ParserConfig m_config;
        std::unique_ptr<Constraints> m_constraints = std::make_unique<Constraints>();

        std::string m_shortPrefix = "-";
        std::string m_longPrefix  = "--";
    public:
        Parser() = default;

        Parser(const Parser&);
        auto operator=(const Parser&) -> Parser&;

        Parser(Parser&&) = default;
        auto operator=(Parser&&) -> Parser& = default;

        template<typename T> requires DerivesFrom<T, IOption>
        explicit Parser(T&& option);

        template<typename T> requires DerivesFrom<T, IOption>
        auto addOption(T&& option) -> void;

        auto addSyntaxError(std::string_view error, int pos, ErrorType type) -> void;

        auto addAnalysisError(std::string_view error, int pos, ErrorType type) -> void;

        auto addAnalysisErrorGroup(std::string_view groupName, int startPos, int endPos) -> void;

        auto removeErrorGroup(int startPos) -> void;

        [[nodiscard]] auto getSyntaxErrors() const -> const ErrorGroup&;

        [[nodiscard]] auto getAnalysisErrors() const -> const ErrorGroup&;

        [[nodiscard]] auto hasErrors() const -> bool;

        [[nodiscard]] auto getHelpMessage() const -> std::string;

        auto printErrors() const -> void;

        auto parse(int argc, const char **argv) -> bool;

        auto parse(std::string_view str) -> bool;

        template<typename T> requires DerivesFrom<T, IOption>
        auto operator|(T&& option) -> Parser&;

        template<typename ValueType>
        auto getOptionValue(std::string_view flag) -> const ValueType&;

        template<typename ValueType>
        auto getOptionValue(std::initializer_list<std::string_view> flagPath) -> const ValueType&;

        template<typename ValueType>
        auto getOptionValue(const FlagPath& flagPath) -> const ValueType&;

        template<typename Container>
        auto getMultiValue(std::string_view flag) -> const Container&;

        template<typename Container>
        auto getMultiValue(std::initializer_list<std::string_view> flagPath) -> const Container&;

        template<typename Container>
        auto getMultiValue(const FlagPath& flagPath) -> const Container&;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue() const;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue(std::string_view groupFlag) const;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue(std::initializer_list<std::string_view> groupPath) const;

        template <typename ValueType, size_t Pos>
        auto getPositionalValue(const FlagPath& groupPath) const;

        auto getConfig() -> ParserConfig&;

        [[nodiscard]] auto getConfig() const -> const ParserConfig&;

        [[nodiscard]] auto constraints() const -> Constraints&;

        auto setDefaultPrefixes(std::string_view shortPrefix, std::string_view longPrefix);

        auto withPositionalPolicy(PositionalPolicy policy) & -> Parser&;

    private:
        auto copyFrom(const Parser& other) -> void;

        auto reset() -> void;

        auto parseStatement() -> StatementAst;

        auto parseOptionBundle(const Ast& parentAst, Context& context) ->
            std::variant<std::monostate, std::unique_ptr<OptionBaseAst>, std::unique_ptr<PositionalAst>>;

        auto parseSingleOption(const Ast& parentAst, Context& context, const Token& flag) -> std::unique_ptr<OptionAst>;

        auto parseMultiOption(const Context& context, const Token& flag) -> std::unique_ptr<MultiOptionAst>;

        auto parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) -> void;

        auto parseOptionGroup(const Ast& parentAst, Context& context, const Token& flag) -> std::unique_ptr<OptionGroupAst>;

        auto getNextValidFlag(const Ast& parentAst, const Context& context, bool printErrors = true) ->
            std::variant<std::monostate, Token, std::unique_ptr<PositionalAst>>;

        auto skipToNextValidFlag(const Ast& parentAst, const Context& context) -> void;

        auto getNextToken() -> Token;

        auto rewindScanner(uint32_t rewindAmount) -> void;

        auto skipScope() -> void;

        auto validateConstraints() -> void;
    };

    template<typename Left, typename Right> requires DerivesFrom<Left, IOption> && DerivesFrom<Right, IOption>
    auto operator|(Left&& left, Right&& right) -> Parser;
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include "Argon/Ast.hpp"
#include "Argon/Attributes.hpp"
#include "Argon/Options/MultiOption.hpp"

namespace Argon {
template<typename T> requires DerivesFrom<T, IOption>
Parser::Parser(T&& option) {
    addOption(std::forward<T>(option));
}

template<typename T> requires DerivesFrom<T, IOption>
auto Parser::addOption(T&& option) -> void {
    m_context->addOption(std::forward<T>(option));
}

inline Parser::Parser(const Parser& other) {
    copyFrom(other);
}

inline auto Parser::operator=(const Parser& other) -> Parser& {
    if (this == &other) return *this;
    copyFrom(other);
    return *this;
}

inline auto Parser::addSyntaxError(const std::string_view error, const int pos, const ErrorType type) -> void {
    m_syntaxErrors.addErrorMessage(error, pos, type);
}

inline auto Parser::addAnalysisError(const std::string_view error, const int pos, const ErrorType type) -> void {
    m_analysisErrors.addErrorMessage(error, pos, type);
}

inline auto Parser::addAnalysisErrorGroup(const std::string_view groupName, const int startPos, const int endPos) -> void {
    m_analysisErrors.addErrorGroup(groupName, startPos, endPos);
}

inline auto Parser::removeErrorGroup(const int startPos) -> void {
    m_analysisErrors.removeErrorGroup(startPos);
}

inline auto Parser::getSyntaxErrors() const -> const ErrorGroup& {
    return m_syntaxErrors;
}

inline auto Parser::getAnalysisErrors() const -> const ErrorGroup& {
    return m_analysisErrors;
}

inline auto Parser::hasErrors() const -> bool {
    return !m_contextValidationErrors.empty() || m_syntaxErrors.hasErrors() ||
            m_analysisErrors.hasErrors() || !m_constraintErrors.empty();
}

inline auto Parser::getHelpMessage() const -> std::string {
    return m_context->getHelpMessage();
}

inline auto Parser::printErrors() const -> void {
    if (!m_contextValidationErrors.empty()) {
        std::cout << "Parser is in an invalid state:\n";
        for (const auto& err : m_contextValidationErrors) {
            std::cout << " - " << err << "\n";
        }
        return;
    }
    if (m_syntaxErrors.hasErrors()) {
        m_syntaxErrors.printErrors();
        return;
    }
    if (m_analysisErrors.hasErrors()) {
        m_analysisErrors.printErrors();
        return;
    }
    if (!m_constraintErrors.empty()) {
        for (const auto& err : m_constraintErrors) {
            std::cout << err << "\n";
        }
    }
}

inline auto Parser::parse(const int argc, const char **argv) -> bool {
    std::string input;
    for (int i = 1; i < argc; i++) {
        input += argv[i];
        input += " ";
    }
    return parse(input);
}

inline auto Parser::parse(const std::string_view str) -> bool {
    m_context->applyPrefixes(m_shortPrefix, m_longPrefix);

    m_context->validate(m_contextValidationErrors);
    if (!m_contextValidationErrors.empty()) {
        return false;
    }

    reset();
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.checkPositionals(*this, *m_context);
    if (m_syntaxErrors.hasErrors()) {
        return false;
    }

    ast.analyze(*this, *m_context);
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
        auto parsed = parseOptionBundle(statement, *m_context);
        std::visit([&statement]<typename T>(T& opt) {
            if constexpr (std::is_same_v<T, std::monostate>) {}
            else {statement.addOption(std::move(opt));}
        }, parsed);
    }
    return statement;
}

inline auto Parser::parseOptionBundle(const Ast& parentAst, Context& context) -> // NOLINT(misc-no-recursion)
    std::variant<std::monostate, std::unique_ptr<OptionBaseAst>, std::unique_ptr<PositionalAst>> {
    auto var = getNextValidFlag(parentAst, context);
    if (std::holds_alternative<std::monostate>(var)) { return std::monostate{}; }
    if (std::holds_alternative<Token>(var)) {
        const auto& flagToken = std::get<Token>(var);

        IOption *iOption = context.getFlagOption(flagToken.image);
        if (dynamic_cast<IsSingleOption *>(iOption)) {
            return parseSingleOption(parentAst, context, flagToken);
        }
        if (dynamic_cast<IsMultiOption *>(iOption)) {
            return parseMultiOption(context, flagToken);
        }
        if (dynamic_cast<OptionGroup *>(iOption)) {
            return parseOptionGroup(parentAst, context, flagToken);
        }

        return std::monostate{};
    }
    if (std::holds_alternative<std::unique_ptr<PositionalAst>>(var)) {
        return std::move(std::get<std::unique_ptr<PositionalAst>>(var));
    }
    return std::monostate{};
}

inline auto Parser::parseSingleOption(const Ast& parentAst, Context& context, const Token& flag)
    -> std::unique_ptr<OptionAst> {
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
            std::format("Expected flag value, got '{}' at position {}", value.image, value.position),
            value.position, ErrorType::Syntax_MissingValue);
        skipToNextValidFlag(parentAst, context);
        return nullptr;
    }

    // If value matches a flag (no value supplied)
    if (context.containsLocalFlag(value.image)) {
        if (&context == &*m_context) {
            m_syntaxErrors.addErrorMessage(
                std::format("No value provided for flag '{}' at position {}", flag.image, flag.position),
                value.position, ErrorType::Syntax_MissingValue
            );
        } else {
            m_syntaxErrors.addErrorMessage(
                std::format("No value provided for flag '{}' inside group '{}' at position {}", flag.image,
                            parentAst.getGroupPath(), flag.position),
                value.position, ErrorType::Syntax_MissingValue
            );
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

inline auto Parser::parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) -> void { //NOLINT (misc-no-recursion)
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
                std::format("No matching ']' found for group '{}'", optionGroupAst.flag.value),
                nextToken.position, ErrorType::Syntax_MissingRightBracket);
            return;
        }
        auto parsed = parseOptionBundle(optionGroupAst, nextContext);
        std::visit([&optionGroupAst]<typename T>(T& opt) {
            if constexpr (std::is_same_v<T, std::monostate>) {}
            else {optionGroupAst.addOption(std::move(opt));}
        }, parsed);
    }
}

inline auto Parser::parseOptionGroup(const Ast& parentAst, Context& context, const Token& flag) -> std::unique_ptr<OptionGroupAst> { //NOLINT (misc-no-recursion)
    const Token lbrack = m_scanner.peekToken();
    if (lbrack.kind != TokenKind::LBRACK) {
        m_syntaxErrors.addErrorMessage(
            std::format("Expected '[', got '{}' at position {}", lbrack.image, lbrack.position),
            lbrack.position, ErrorType::Syntax_MissingLeftBracket);
        skipToNextValidFlag(parentAst, context);
        return nullptr;
    }
    getNextToken();

    const auto optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    auto& nextContext = optionGroup->getContext();

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);
    parseGroupContents(*optionGroupAst, nextContext);
    return optionGroupAst;
}

inline auto Parser::getNextValidFlag(const Ast& parentAst, const Context& context, const bool printErrors) ->
    std::variant<std::monostate, Token, std::unique_ptr<PositionalAst>> {
    Token flag = m_scanner.peekToken();

    const bool isIdentifier     = flag.kind == TokenKind::IDENTIFIER;
    const bool hasFlagPrefix    = flag.image.starts_with(m_shortPrefix) || flag.image.starts_with(m_longPrefix);
    const bool inContext        = context.containsLocalFlag(flag.image);
    const bool isPositional     = isIdentifier && !hasFlagPrefix;

    // Is a positional arg
    if (isPositional) {
        getNextToken();
        return std::make_unique<PositionalAst>(flag);
    }

    // Is a valid flag in the context
    if (isIdentifier && inContext) {
        getNextToken();
        return flag;
    }

    if (printErrors && !isIdentifier) {
        m_syntaxErrors.addErrorMessage(
            std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position),
            flag.position, ErrorType::Syntax_MissingFlagName
        );
    } else if (printErrors) {
        if (&context == &*m_context) {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}' at position {}", flag.image, flag.position),
                flag.position, ErrorType::Syntax_UnknownFlag
            );
        } else {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}' inside group '{}' at position {}", flag.image, parentAst.getGroupPath(),
                            flag.position),
                flag.position, ErrorType::Syntax_UnknownFlag
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
        }
        if (m_mismatchedRBRACK) {
            getNextToken();
            continue;
        }
        if (token.kind == TokenKind::RBRACK || token.kind == TokenKind::END) {
            // Escape this scope, leave RBRACK scanning to the function above
            return std::monostate{};
        }
        if (token.kind == TokenKind::IDENTIFIER && context.containsLocalFlag(token.image)) {
            getNextToken();
            return token;
        }
        getNextToken();
    }
}

inline auto Parser::skipToNextValidFlag(const Ast& parentAst, const Context& context) -> void {
    getNextValidFlag(parentAst, context, false);
    if (m_scanner.peekToken().kind != TokenKind::END) {
        rewindScanner(1);
    }
}

template <typename T> requires DerivesFrom<T, IOption>
auto Parser::operator|(T&& option) -> Parser& {
    addOption(std::forward<T>(option));
    return *this;
}

template<typename ValueType>
auto Parser::getOptionValue(const std::string_view flag) -> const ValueType& {
    return m_context->getOptionValue<ValueType>(FlagPath(flag));
}

template<typename ValueType>
auto Parser::getOptionValue(const std::initializer_list<std::string_view> flagPath) -> const ValueType& {
    return m_context->getOptionValue<ValueType>(flagPath);
}

template<typename ValueType>
auto Parser::getOptionValue(const FlagPath& flagPath) -> const ValueType& {
    return m_context->getOptionValue<ValueType>(flagPath);
}

template<typename Container>
auto Parser::getMultiValue(const std::string_view flag) -> const Container& {
    return m_context->getMultiValue<Container>(FlagPath(flag));
}

template<typename Container>
auto Parser::getMultiValue(const std::initializer_list<std::string_view> flagPath) -> const Container& {
    return m_context->getMultiValue<Container>(flagPath);
}

template<typename Container>
auto Parser::getMultiValue(const FlagPath& flagPath) -> const Container& {
    return m_context->getMultiValue<Container>(flagPath);
}

template<typename ValueType, size_t Pos>
auto Parser::getPositionalValue() const {
    return m_context->getPositionalValue<ValueType, Pos>();
}

template<typename ValueType, size_t Pos>
auto Parser::getPositionalValue(const std::string_view groupFlag) const {
    return m_context->getPositionalValue<ValueType, Pos>(FlagPath(groupFlag));
}

template<typename ValueType, size_t Pos>
auto Parser::getPositionalValue(std::initializer_list<std::string_view> groupPath) const {
    return m_context->getPositionalValue<ValueType, Pos>(groupPath);
}

template<typename ValueType, size_t Pos>
auto Parser::getPositionalValue(const FlagPath& groupPath) const {
    return m_context->getPositionalValue<ValueType, Pos>(groupPath);
}

inline auto Parser::getConfig() -> ParserConfig& {
    return m_config;
}

inline auto Parser::getConfig() const -> const ParserConfig& {
    return m_config;
}

inline auto Parser::constraints() const -> Constraints& {
    return *m_constraints;
}

inline auto Parser::setDefaultPrefixes(const std::string_view shortPrefix, const std::string_view longPrefix) {
    m_shortPrefix = shortPrefix;
    m_longPrefix = longPrefix;
}

inline auto Parser::withPositionalPolicy(const PositionalPolicy policy) & -> Parser& {
    m_context->setPositionalPolicy(policy);
    return *this;
}

inline auto Parser::copyFrom(const Parser& other) -> void {
    m_context = std::make_unique<Context>(*other.m_context);
    m_scanner = other.m_scanner;

    m_contextValidationErrors   = other.m_contextValidationErrors;
    m_syntaxErrors              = other.m_syntaxErrors;
    m_analysisErrors            = other.m_analysisErrors;
    m_constraintErrors          = other.m_constraintErrors;

    m_brackets                  = other.m_brackets;
    m_poppedBrackets            = other.m_poppedBrackets;
    m_mismatchedRBRACK          = other.m_mismatchedRBRACK;

    m_config      = other.m_config;
    m_constraints = std::make_unique<Constraints>(*other.m_constraints);

    m_shortPrefix = other.m_shortPrefix;
    m_longPrefix  = other.m_longPrefix;
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
                nextToken.position, ErrorType::Syntax_MissingLeftBracket
            );
        } else {
            m_poppedBrackets.push_back(m_brackets.back());
            m_brackets.pop_back();
        }
    }
    return nextToken;
}

inline auto Parser::rewindScanner(const uint32_t rewindAmount) -> void {
    for (const auto& token : std::ranges::reverse_view(m_scanner.rewind(rewindAmount))) {
        if (token.kind == TokenKind::RBRACK && !m_poppedBrackets.empty()) {
            m_brackets.push_back(m_poppedBrackets.back());
            m_poppedBrackets.pop_back();
        }
    }
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
                    std::format("Unmatched ']' found at position {}", token.position),
                    token.position, ErrorType::Syntax_MissingLeftBracket);
                return;
            }
            brackets.pop_back();
        }

        if (token.kind == TokenKind::END && !brackets.empty()) {
            for (const auto& bracket: brackets) {
                m_syntaxErrors.addErrorMessage(
                    std::format("Unmatched '[' found at position {}", bracket.position),
                    bracket.position, ErrorType::Syntax_MissingRightBracket);
            }
            return;
        }

        if (brackets.empty()) {
            return;
        }
    }
}

inline auto Parser::validateConstraints() -> void {
    m_constraints->validate(*m_context, m_constraintErrors);
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