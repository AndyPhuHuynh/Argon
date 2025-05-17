#pragma once

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
        StatementAst parseStatement();
        std::unique_ptr<OptionBaseAst> parseOption(Context& context);

        void handleBadGroupFlag(const Token& flag);
        void parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext);
        std::unique_ptr<OptionGroupAst> parseOptionGroup(Context& context);

        Token expectFlagToken();
        Token expectToken(TokenKind kind, const std::format_string<std::string&, int&>& errorMsg);
        Token expectToken(std::initializer_list<TokenKind> kinds, const std::format_string<std::string&, int&>& errorMsg);
        bool checkValidFlag(const Context& context, Token& flag, const std::function<void()> &onErr);
        template <typename T>
        bool checkFlagType(Context& context, Token& flag, const std::format_string<std::string&, int&>& errorMsg, const std::function<void()>& onErr);
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
    m_syntaxErrors.clear();
    m_analysisErrors.clear();
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(m_analysisErrors, m_context);
    m_syntaxErrors.printErrorsTreeMode();
    m_analysisErrors.printErrorsTreeMode();
    // m_analysisErrors.printErrorsFlatMode();
}

inline Argon::StatementAst Argon::Parser::parseStatement() {
    StatementAst statement;
    bool stop = false;
    while (!stop) {
        m_scanner.recordPosition();

        // Get first token
        Token token1 = m_scanner.getNextToken();

        bool token1Error = false;
        if (token1.kind != TokenKind::IDENTIFIER) {
            token1Error = true;
            m_syntaxErrors.addErrorMessage(
                std::format("Expected option or option group, got '{}' at position {}", token1.image, token1.position),
                token1.position);
            m_scanner.scanUntilSee({TokenKind::IDENTIFIER});
            m_scanner.recordPosition();
            token1 = m_scanner.getNextToken();
        }

        if (token1.kind == TokenKind::END) {
            break;
        }

        // Get second token
        Token token2 = m_scanner.getNextToken();

        if (token2.kind != TokenKind::IDENTIFIER && token2.kind != TokenKind::LBRACK) {
            if (!token1Error) {
                m_syntaxErrors.addErrorMessage(
                    std::format("Expected option or option group, got '{}' at position {}", token2.image, token2.position),
                    token2.position);
            }
            token2 = m_scanner.scanUntilGet({TokenKind::IDENTIFIER, TokenKind::LBRACK});
        }

        if (token2.kind == TokenKind::END) {
            break;
        }

        // OptionGroup
        if (token2.kind == TokenKind::LBRACK) {
            m_scanner.rewind();
            statement.addOption(parseOptionGroup(m_context));
        }
        // Option
        else if (token2.kind == TokenKind::IDENTIFIER) {
            m_scanner.rewind();
            statement.addOption(parseOption(m_context));
        }

        stop = m_scanner.seeTokenKind(TokenKind::END);
    }
    return statement;
}

inline std::unique_ptr<Argon::OptionBaseAst> Argon::Parser::parseOption(Context& context) {
    // Get flag
    Token flag = expectFlagToken();

    // If the flag is not valid, scan until we find a valid flag
    if (!context.containsLocalFlag(flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);

        while (true) {
            const Token nextToken = m_scanner.peekToken();

            const bool isFlag = nextToken.kind == TokenKind::IDENTIFIER && context.containsLocalFlag(nextToken.image);
            const bool notIdentifier = nextToken.kind != TokenKind::IDENTIFIER;
            if (isFlag || notIdentifier) {
                return nullptr;
            }

            m_scanner.getNextToken();
        }
    }

    // If the flag is an option group
    if (context.getOptionDynamic<OptionGroup>(flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Flag '{}' at position {} is not a single or multi option", flag.image, flag.position), flag.position);
        return nullptr;
    }

    // We now have a valid flag
    OptionBase *option = context.getOptionDynamic<OptionBase>(flag.image);
    if (dynamic_cast<IsSingleOption*>(option)) {
        // Get value
        m_scanner.recordPosition();
        Token value = m_scanner.getNextToken();
        if (value.kind != TokenKind::IDENTIFIER) {
            m_syntaxErrors.addErrorMessage(std::format("Expected flag value, got '{}' at position {}", value.image, value.position), value.position);
            value = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
        }

        // If value matches a flag
        if (context.containsLocalFlag(value.image)) {
            m_syntaxErrors.addErrorMessage(std::format("No value provided for flag '{}' at position {}", flag.image, flag.position), value.position);
            m_scanner.rewind();
        }

        return std::make_unique<OptionAst>(flag, value);
    } else if (dynamic_cast<IsMultiOption*>(option)) {
        auto multiOptionAst = std::make_unique<MultiOptionAst>(flag);

        while (true) {
            Token nextToken = m_scanner.peekToken();

            const bool endOfMultiOption = nextToken.kind != TokenKind::IDENTIFIER || context.containsLocalFlag(nextToken.image);
            if (endOfMultiOption) {
                return multiOptionAst;
            } else {
                multiOptionAst->addValue(nextToken);
                m_scanner.getNextToken();
            }
        }
    }
    m_syntaxErrors.addErrorMessage(
        std::format("Expected option or multi-option, got {} at position {}", flag.image, flag.position),
        flag.position);
    return nullptr;
}

inline void Argon::Parser::handleBadGroupFlag(const Token& flag) {
    expectToken(TokenKind::LBRACK, "Expected '[', got '{}' at position {}");
    const Token end = m_scanner.scanUntilGet({TokenKind::RBRACK});
    if (end.kind == TokenKind::END) {
        m_syntaxErrors.addErrorMessage(std::format("No matching ']' found for flag {} at position {}", flag.image, flag.position), end.position);
    }
}

inline void Argon::Parser::parseGroupContents(OptionGroupAst& optionGroupAst, Context& nextContext) { //NOLINT (recursion)
    while (true) {
        // Get flag
        m_scanner.recordPosition();
        const Token token1 = expectToken(
            {TokenKind::IDENTIFIER, TokenKind::RBRACK, TokenKind::END},
            "Expected flag name or ']', got '{}' at position {}"
        );

        if (token1.kind == TokenKind::RBRACK) {
            optionGroupAst.endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroupAst.flag.value, optionGroupAst.flag.pos, optionGroupAst.endPos);
            return;
        }

        if (token1.kind == TokenKind::END) {
            optionGroupAst.endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroupAst.flag.value, optionGroupAst.flag.pos, optionGroupAst.endPos);
            m_syntaxErrors.addErrorMessage(std::format("No matching ']' found for group '{}'", optionGroupAst.flag.value), token1.position);
            return;
        }

        // Get the value for option or the lbrack for option group
        Token token2 = expectToken(
            {TokenKind::IDENTIFIER, TokenKind::LBRACK, TokenKind::END},
            "Expected option value or '[', got {} at position {}"
        );

        if (token2.kind == TokenKind::END) {
            m_syntaxErrors.addErrorMessage(
                std::format("Expected option or option group, got {} at position {}", token2.image, token2.position),
                token2.position
            );
            m_syntaxErrors.addErrorMessage(
                std::format("No matching ']' found for group '{}'", optionGroupAst.flag.value),
                token1.position
            );
            optionGroupAst.endPos = token2.position;
            return;
        }

        m_scanner.rewind();
        if (token2.kind == TokenKind::IDENTIFIER) {
            optionGroupAst.addOption(parseOption(nextContext));
        } else {
            optionGroupAst.addOption(parseOptionGroup(nextContext));
        }
    }
}

inline std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup(Context& context) { //NOLINT (recursion)
    Token flag = expectFlagToken();
    auto onBadFlag = [&] -> void { handleBadGroupFlag(flag); };

    if (!checkValidFlag(context, flag, onBadFlag)) return nullptr;
    if (!checkFlagType<OptionGroup>(context, flag, "Flag '{}' at position {} is not an option group", onBadFlag)) return nullptr;
    expectToken(TokenKind::LBRACK, "Expected '[', got '{}' at position {}");

    const auto optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    auto& nextContext = optionGroup->get_context();

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);
    parseGroupContents(*optionGroupAst, nextContext);
    return optionGroupAst;
}

inline Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}

inline Argon::Token Argon::Parser::expectFlagToken() {
    return expectToken(TokenKind::IDENTIFIER, "Expected flag name, got '{}' at position {}");
}

inline Argon::Token Argon::Parser::expectToken(TokenKind kind, const std::format_string<std::string&, int&>& errorMsg) {
    Token token = m_scanner.getNextToken();
    if (token.kind != kind) {
        m_syntaxErrors.addErrorMessage(std::format(errorMsg, token.image, token.position), token.position);
        token = m_scanner.scanUntilGet({kind});
    }
    return token;
}

inline Argon::Token Argon::Parser::expectToken(std::initializer_list<TokenKind> kinds,
    const std::format_string<std::string &, int &> &errorMsg) {
    Token token = m_scanner.getNextToken();
    if (!token.isOneOf(kinds)) {
        m_syntaxErrors.addErrorMessage(std::format(errorMsg, token.image, token.position), token.position);
        token = m_scanner.scanUntilGet({kinds});
    }
    return token;
}

inline bool Argon::Parser::checkValidFlag(const Context &context, Token &flag, const std::function<void()> &onErr) {
    if (!context.containsLocalFlag(flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);
        onErr();
        return false;
    }
    return true;
}

template<typename T>
bool Argon::Parser::checkFlagType(Context& context, Token &flag, const std::format_string<std::string&, int&>& errorMsg, const std::function<void()> &onErr) {
    if (context.getOptionDynamic<T>(flag.image) == nullptr) {
        m_analysisErrors.addErrorMessage(std::format(errorMsg, flag.image, flag.position), flag.position);
        onErr();
        return false;
    }
    return true;
}

inline Argon::Parser Argon::operator|(const IOption &left, const IOption &right) {
    Parser parser;
    parser.addOption(left);
    parser.addOption(right);
    return parser;
}
