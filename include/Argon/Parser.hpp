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
        StatementAst parseStatement();
        std::unique_ptr<OptionBaseAst> parseOption(Context& context);
        std::unique_ptr<OptionGroupAst> parseOptionGroup(Context& context);
        
        Parser& operator|(const IOption& option);
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
                std::format("Expected option or option group, got {} at position {}", token1.image, token1.position),
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
                    std::format("Expected option or option group, got {} at position {}", token2.image, token2.position),
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
    Token flag = m_scanner.getNextToken();
    if (flag.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position), flag.position);
        flag = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

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

inline std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup(Context& context) {
    // Get flag
    Token flag = m_scanner.getNextToken();
    if (flag.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position), flag.position);
        flag = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

    bool validFlag = true;
    // If the flag is not valid
    if (!context.containsLocalFlag(flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);
        validFlag = false;
    }

    OptionGroup *optionGroup = context.getOptionDynamic<OptionGroup>(flag.image);
    if (optionGroup == nullptr) {
        if (validFlag) {
            m_analysisErrors.addErrorMessage(std::format("Flag '{}' at position {} is not an option group", flag.image, flag.position), flag.position);
        }
        validFlag = false;
    }

    // Get lbrack
    Token lbrack = m_scanner.getNextToken();
    if (lbrack.kind != TokenKind::LBRACK) {
        m_syntaxErrors.addErrorMessage(std::format("Expected '[', got '{}' at position {}", lbrack.image, lbrack.position), lbrack.position);
        m_scanner.scanUntilGet({TokenKind::LBRACK});
    }

    // Early return if invalid flag
    if (!validFlag) {
        const Token end = m_scanner.scanUntilGet({TokenKind::RBRACK});
        if (end.kind == TokenKind::END) {
            m_syntaxErrors.addErrorMessage("No matching ']' found for flag " + flag.image, end.position);
            m_syntaxErrors.addErrorMessage(std::format("No matching ']' found for flag {} at position {}", flag.image, flag.position), end.position);
        }
        return nullptr;
    }

    auto optionGroupAst = std::make_unique<OptionGroupAst>(flag);

    // TODO: Right now, this can immediately stop if there is '[]' with no options inside
    while (true) {
        Context& nextContext = optionGroup->get_context();

        // Get flag
        m_scanner.recordPosition();
        Token token1 = m_scanner.getNextToken();

        if (token1.kind != TokenKind::RBRACK && token1.kind != TokenKind::IDENTIFIER) {
            m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", token1.image, token1.position), token1.position);
            m_scanner.scanUntilGet({TokenKind::IDENTIFIER, TokenKind::LBRACK});
        }

        if (token1.kind == TokenKind::RBRACK) {
            optionGroupAst->endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroupAst->flag.value, optionGroupAst->flag.pos, optionGroupAst->endPos);
            return optionGroupAst;
        } else if (token1.kind == TokenKind::END) {
            optionGroupAst->endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroupAst->flag.value, optionGroupAst->flag.pos, optionGroupAst->endPos);
            m_syntaxErrors.addErrorMessage("No matching ']' found for group " + flag.image, token1.position);
            return optionGroupAst;
        }

        // Get value for option/lbrack for option group
        Token token2 = m_scanner.getNextToken();

        if (token2.kind != TokenKind::LBRACK && token2.kind != TokenKind::IDENTIFIER) {
            m_syntaxErrors.addErrorMessage(std::format("Expected option value or '[', got {} at position {}", token2.image, token2.position), token2.position);
            m_scanner.scanUntilGet({TokenKind::IDENTIFIER, TokenKind::LBRACK});
        }

        if (token2.kind == TokenKind::IDENTIFIER) {
            m_scanner.rewind();
            optionGroupAst->addOption(parseOption(nextContext));
        } else if (token2.kind == TokenKind::LBRACK) {
            m_scanner.rewind();
            optionGroupAst->addOption(parseOptionGroup(nextContext));
        } else if (token2.kind == TokenKind::END) {
            m_syntaxErrors.addErrorMessage(
                    std::format("Expected option or option group, got {} at position {}", token2.image, token2.position),
                    token2.position);
            m_syntaxErrors.addErrorMessage("No matching ']' found for group " + flag.image, token1.position);
            optionGroupAst->endPos = token2.position;
            return optionGroupAst;
        }
    }
}

inline Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}

inline Argon::Parser Argon::operator|(const IOption &left, const IOption &right) {
    Parser parser;
    parser.addOption(left);
    parser.addOption(right);
    return parser;
}
