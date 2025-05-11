#include "Argon/Parser.hpp"

#include <algorithm>

#include "Argon/Option.hpp"

void Argon::Parser::addOption(const IOption& option) {
    m_context.addOption(option);
 }

void Argon::Parser::addError(const std::string& error, const int pos) {
    m_analysisErrors.addErrorMessage(error, pos);
}

void Argon::Parser::addErrorGroup(const std::string& groupName, const int startPos, const int endPos) {
    m_analysisErrors.addErrorGroup(groupName, startPos, endPos);
}

void Argon::Parser::removeErrorGroup(const int startPos) {
    m_analysisErrors.removeErrorGroup(startPos);
}

void Argon::Parser::parseString(const std::string& str) {
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(*this, m_context);
    m_syntaxErrors.printErrorsTreeMode();
    m_analysisErrors.printErrorsTreeMode();
    // m_analysisErrors.printErrorsFlatMode();
}

Argon::StatementAst Argon::Parser::parseStatement() {
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

std::unique_ptr<Argon::OptionAst> Argon::Parser::parseOption(Context& context) {
    bool error = false;
    // Get flag
    Token flag = m_scanner.getNextToken();
    if (flag.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position), flag.position);
        flag = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

    // If the flag is not valid
    if (!context.containsLocalFlag(flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);
        error = true;
    }
    
    if (OptionBase *option = context.getOptionDynamic<OptionBase>(flag.image)) {
        
    }
    
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
        error = true;
    }
    
    return error ? nullptr : std::make_unique<OptionAst>(flag, value);
}

std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup(Context& context) {
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
    if (optionGroup == nullptr && validFlag) {
        m_analysisErrors.addErrorMessage(std::format("Flag '{}' at position {} is not an option group", flag.image, flag.position), flag.position);
        validFlag = false;
    }
    
    // Get lbrack
    Token lbrack = m_scanner.getNextToken();
    if (lbrack.kind != TokenKind::LBRACK) {
        m_syntaxErrors.addErrorMessage(std::format("Expected '[', got '{}' at position {}", lbrack.image, lbrack.position), lbrack.position);
        lbrack = m_scanner.scanUntilGet({TokenKind::LBRACK});
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
            return optionGroupAst;
        }
    }
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}
