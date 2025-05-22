#pragma once

#include <cassert>
#include <memory>
#include <string>

#include "Ast.hpp"
#include "Context.hpp"
#include "Error.hpp"
#include "Scanner.hpp"

namespace Argon {
    class Parser {
        Context m_context;
        Scanner m_scanner = Scanner();

        ErrorGroup m_syntaxErrors = ErrorGroup("Syntax Errors", -1, -1);
        ErrorGroup m_analysisErrors = ErrorGroup("Analysis Errors", -1, -1);
        std::vector<Token> m_brackets;
    public:
        Parser() = default;
        explicit Parser(const IOption& option);

        void addOption(const IOption& option);

        void addError(const std::string& error, int pos);
        void addErrorGroup(const std::string& groupName, int startPos, int endPos);
        void removeErrorGroup(int startPos);
        
        void parseString(const std::string& str);
        Parser& operator|(const IOption& option);
    private:
        void reset();
        StatementAst parseStatement();

        auto parseOptionBundle(Context& context) -> std::unique_ptr<OptionBaseAst>;

        void handleBadOptionFlag(const Context& context);
        std::unique_ptr<OptionAst> parseSingleOption(const Context& context, const Token& flag);
        std::unique_ptr<MultiOptionAst> parseMultiOption(const Context& context, const Token& flag);
        std::unique_ptr<OptionBaseAst> parseOption(Context& context);

        void handleBadGroupFlag(const Token& flag);
        void parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext);
        std::unique_ptr<OptionGroupAst> parseOptionGroup(Context& context, const Token& flag);

        auto getNextValidFlag(const Context &context, bool printErrors = true) -> std::optional<Token>;
        Token expectFlag();
        Token expectToken(TokenKind kind, const std::format_string<std::string&, int&>& errorMsg);
        Token expectToken(std::initializer_list<TokenKind> kinds, const std::format_string<std::string&, int&>& errorMsg);

        Token getNextToken();
        void scanUntilSee(const std::initializer_list<TokenKind>& kinds);
        Token scanUntilGet(const std::initializer_list<TokenKind>& kinds);
        void skipScope();
    };

    Parser operator|(const IOption& left, const IOption& right);
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include "MultiOption.hpp"

inline Argon::Parser::Parser(const IOption &option) {
    addOption(option);
}

inline void Argon::Parser::addOption(const IOption& option) {
    m_context.addOption(option);
 }

inline void Argon::Parser::addError(const std::string& error, const int pos) {
    m_analysisErrors.addErrorMessage(error, pos);
}

inline void Argon::Parser::addErrorGroup(const std::string& groupName, const int startPos, const int endPos) {
    m_analysisErrors.addErrorGroup(groupName, startPos, endPos);
}

inline void Argon::Parser::removeErrorGroup(const int startPos) {
    m_analysisErrors.removeErrorGroup(startPos);
}

inline void Argon::Parser::parseString(const std::string& str) {
    reset();
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(m_analysisErrors, m_context);
    m_syntaxErrors.printErrorsTreeMode();
    m_analysisErrors.printErrorsTreeMode();
    // m_analysisErrors.printErrorsFlatMode();
}

inline Argon::StatementAst Argon::Parser::parseStatement() {
    StatementAst statement;
    while (!m_scanner.seeTokenKind(TokenKind::END)) {
        statement.addOption(parseOptionBundle(m_context));
    }
    return statement;
}

inline auto Argon::Parser::parseOptionBundle(Context& context) -> std::unique_ptr<OptionBaseAst> { // NOLINT(misc-no-recursion)
    const auto opt = getNextValidFlag(context);
    if (!opt.has_value()) { return nullptr; }
    const auto& flagToken = opt.value();

    IOption *iOption = context.getOption(flagToken.image);
    if (dynamic_cast<IsSingleOption*>(iOption)) {
        return parseSingleOption(context, flagToken);
    }
    if (dynamic_cast<IsMultiOption*>(iOption)) {
        return parseMultiOption(context, flagToken);
    }
    if (dynamic_cast<OptionGroup*>(iOption)) {
        return parseOptionGroup(context, flagToken);
    }

    return nullptr;
}

inline std::unique_ptr<Argon::OptionAst> Argon::Parser::parseSingleOption(const Context &context, const Token& flag) {
    // Get value
    Token value = m_scanner.peekToken();
    if (value.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag value, got '{}' at position {}", value.image, value.position), value.position);
        getNextValidFlag(context, false);
        m_scanner.rewind(1);
        return nullptr;
    }

    // If value matches a flag (no value supplied)
    if (context.containsLocalFlag(value.image)) {
        m_syntaxErrors.addErrorMessage(std::format("No value provided for flag '{}' at position {}", flag.image, flag.position), value.position);
        getNextValidFlag(context, false);
        m_scanner.rewind(1);
        return nullptr;
    }

    getNextToken();
    return std::make_unique<OptionAst>(flag, value);
}

inline std::unique_ptr<Argon::MultiOptionAst> Argon::Parser::parseMultiOption(const Context &context, const Token &flag) {
    auto multiOptionAst = std::make_unique<MultiOptionAst>(flag);

    while (true) {
        Token nextToken = m_scanner.peekToken();

        const bool endOfMultiOption = nextToken.kind != TokenKind::IDENTIFIER || context.containsLocalFlag(nextToken.image);
        if (endOfMultiOption) {
            return multiOptionAst;
        }

        multiOptionAst->addValue(nextToken);
        getNextToken();
    }
}

inline void Argon::Parser::parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) { // NOLINT(misc-no-recursion)
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
            m_syntaxErrors.addErrorMessage(std::format("No matching ']' found for group '{}'", optionGroupAst.flag.value), nextToken.position);
            return;
        }

        optionGroupAst.addOption(parseOptionBundle(nextContext));
    }
}

inline std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup(Context& context, const Token& flag) { // NOLINT(misc-no-recursion)
    expectToken(TokenKind::LBRACK, "Expected '[', got '{}' at position {}");

    const auto optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    auto& nextContext = optionGroup->get_context();

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);
    parseGroupContents(*optionGroupAst, nextContext);
    return optionGroupAst;
}

inline auto Argon::Parser::getNextValidFlag(const Context& context, bool printErrors) -> std::optional<Token> {
    Token flag = m_scanner.peekToken();

    const bool isIdentifier =  flag.kind == TokenKind::IDENTIFIER;
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
        m_syntaxErrors.addErrorMessage(
            std::format("Unknown flag '{}' at position {}", flag.image, flag.position),
            flag.position
        );
    }

    if (flag.kind == TokenKind::LBRACK) {
        skipScope();
    }

    while (true) {
        Token token = m_scanner.peekToken();
        if (token.kind == TokenKind::LBRACK) {
            skipScope();
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

inline Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}

inline void Argon::Parser::reset() {
    m_syntaxErrors.clear();
    m_analysisErrors.clear();
    m_brackets.clear();
}

inline Argon::Token Argon::Parser::expectFlag() {
    return expectToken(TokenKind::IDENTIFIER, "Expected flag name, got '{}' at position {}");
}

inline Argon::Token Argon::Parser::expectToken(TokenKind kind, const std::format_string<std::string&, int&>& errorMsg) {
    Token token = getNextToken();
    if (token.kind != kind) {
        m_syntaxErrors.addErrorMessage(std::format(errorMsg, token.image, token.position), token.position);
        token = scanUntilGet({kind});
    }
    return token;
}

inline Argon::Token Argon::Parser::expectToken(std::initializer_list<TokenKind> kinds,
    const std::format_string<std::string &, int &> &errorMsg) {
    Token token = getNextToken();
    if (!token.isOneOf(kinds)) {
        m_syntaxErrors.addErrorMessage(std::format(errorMsg, token.image, token.position), token.position);
        token = scanUntilGet({kinds});
    }
    return token;
}

inline Argon::Token Argon::Parser::getNextToken() {
    const Token nextToken = m_scanner.getNextToken();
    // if (nextToken.kind == TokenKind::LBRACK) {
    //     m_brackets.push_back(nextToken);
    // } else if (nextToken.kind == TokenKind::RBRACK) {
    //     if (m_brackets.empty()) {
    //         m_syntaxErrors.addErrorMessage(
    //             std::format("No matching '[' found for ']' at positions {}", nextToken.position),
    //             nextToken.position
    //         );
    //     } else {
    //         m_brackets.pop_back();
    //     }
    // }
    return nextToken;
}

inline void Argon::Parser::scanUntilSee(const std::initializer_list<TokenKind>& kinds) {
    while (!m_scanner.seeTokenKind(kinds) && !m_scanner.seeTokenKind(TokenKind::END)) {
        if (m_scanner.seeTokenKind(TokenKind::LBRACK)) {
            skipScope();
        }
    }
}

inline Argon::Token Argon::Parser::scanUntilGet(const std::initializer_list<TokenKind>& kinds) {
    scanUntilSee(kinds);
    return getNextToken();
}

inline void Argon::Parser::skipScope() {
    if (m_scanner.peekToken().kind != TokenKind::LBRACK) return;
    std::vector<Token> brackets;
    while (true) {
        const Token token = m_scanner.getNextToken();
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
            for (const auto& bracket : brackets) {
                m_syntaxErrors.addErrorMessage(
                    std::format("Unmatched '[' found at position {}", bracket.position), bracket.position);
            }
        }

        if (brackets.empty()) {
            return;
        }
    }
}

inline Argon::Parser Argon::operator|(const IOption &left, const IOption &right) {
    Parser parser;
    parser.addOption(left);
    parser.addOption(right);
    return parser;
}
