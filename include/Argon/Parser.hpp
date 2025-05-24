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
        Scanner m_scanner;

        ErrorGroup m_syntaxErrors = ErrorGroup("Syntax Errors", -1, -1);
        ErrorGroup m_analysisErrors = ErrorGroup("Analysis Errors", -1, -1);

        std::vector<Token> m_brackets;
        bool m_mismatchedRBRACK = false;
    public:
        Parser() = default;
        explicit Parser(const IOption& option);

        void addOption(const IOption& option);

        void addError(const std::string& error, int pos);
        void addErrorGroup(const std::string& groupName, int startPos, int endPos);
        void removeErrorGroup(int startPos);
        [[nodiscard]] bool hasErrors() const;
        void printErrors(PrintMode analysisPrintMode, TextMode analysisTextMode = TextMode::Ascii) const;
        
        void parseString(const std::string& str);
        Parser& operator|(const IOption& option);
    private:
        auto reset() -> void;

        auto parseStatement() -> StatementAst;

        auto parseOptionBundle(Context& context) -> std::unique_ptr<OptionBaseAst>;

        auto parseSingleOption(const Context &context, const Token &flag) -> std::unique_ptr<OptionAst>;
        auto parseMultiOption(const Context &context, const Token &flag) -> std::unique_ptr<MultiOptionAst>;

        auto parseGroupContents(OptionGroupAst &optionGroupAst, Context &nextContext) -> void;
        auto parseOptionGroup(Context &context, const Token &flag) -> std::unique_ptr<OptionGroupAst>;

        auto getNextValidFlag(const Context &context, bool printErrors = true) -> std::optional<Token>;

        auto getNextToken() -> Token;
        auto skipScope() -> void;
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

inline bool Argon::Parser::hasErrors() const {
    return m_syntaxErrors.hasErrors() || m_analysisErrors.hasErrors();
}

inline void Argon::Parser::printErrors(const PrintMode analysisPrintMode, const TextMode analysisTextMode) const {
    if (m_syntaxErrors.hasErrors()) {
        m_syntaxErrors.printErrorsFlatMode();
        return;
    }
    if (!m_analysisErrors.hasErrors()) return;

    switch (analysisPrintMode) {
        case PrintMode::Flat:
            m_analysisErrors.printErrorsFlatMode();
            return;
        case PrintMode::Tree:
            m_analysisErrors.printErrorsTreeMode(analysisTextMode);
    }
}

inline void Argon::Parser::parseString(const std::string& str) {
    reset();
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(m_analysisErrors, m_context);
    if (hasErrors()) {
        printErrors(PrintMode::Flat, TextMode::Utf8);
    }
}

inline auto Argon::Parser::parseStatement() -> StatementAst {
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

inline auto Argon::Parser::parseSingleOption(const Context &context, const Token &flag) -> std::unique_ptr<OptionAst> {
    // Get value
    Token value = m_scanner.peekToken();
    if (value.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag value, got '{}' at position {}", value.image, value.position), value.position);
        getNextValidFlag(context, false);
        if (m_scanner.peekToken().kind != TokenKind::END) {
            m_scanner.rewind(1);
        }
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
                std::format("No value provided for flag '{}' inside group '{}' at position {}", flag.image, context.getPath(), flag.position),
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

inline auto Argon::Parser::parseMultiOption(const Context &context, const Token &flag) -> std::unique_ptr<MultiOptionAst> {
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

inline auto Argon::Parser::parseGroupContents(OptionGroupAst &optionGroupAst, Context &nextContext) -> void { // NOLINT(misc-no-recursion)
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

inline auto Argon::Parser::parseOptionGroup(Context &context, const Token &flag) -> std::unique_ptr<OptionGroupAst> { // NOLINT(misc-no-recursion)
    const Token lbrack = m_scanner.peekToken();
    if (lbrack.kind != TokenKind::LBRACK) {
        m_syntaxErrors.addErrorMessage(std::format("Expected '[', got '{}' at position {}", lbrack.image, lbrack.position), lbrack.position);
        return nullptr;
    }
    getNextToken();

    const auto optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    auto& nextContext = optionGroup->get_context();
    nextContext.setName(flag.image);

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);
    parseGroupContents(*optionGroupAst, nextContext);
    return optionGroupAst;
}

inline auto Argon::Parser::getNextValidFlag(const Context& context, const bool printErrors) -> std::optional<Token> {
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
        if (&context == &m_context) {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}'  at position {}", flag.image, flag.position),
                flag.position
            );
        } else {
            m_syntaxErrors.addErrorMessage(
                std::format("Unknown flag '{}' inside group '{}' at position {}", flag.image, context.getPath(), flag.position),
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

inline auto Argon::Parser::operator|(const IOption &option) -> Parser& {
    addOption(option);
    return *this;
}

inline auto Argon::Parser::reset() -> void {
    m_syntaxErrors.clear();
    m_analysisErrors.clear();
    m_brackets.clear();
}

inline auto Argon::Parser::getNextToken() -> Token {
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

inline auto Argon::Parser::skipScope() -> void {
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
            for (const auto& bracket : brackets) {
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

inline Argon::Parser Argon::operator|(const IOption &left, const IOption &right) {
    Parser parser;
    parser.addOption(left);
    parser.addOption(right);
    return parser;
}
