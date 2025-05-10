#include "Argon/Parser.hpp"

#include <algorithm>

#include "Argon/Option.hpp"

Argon::Parser::Parser(const Parser& parser) {
    for (auto& option : parser.m_options) {
        this->m_options.push_back(option->clone());
    }
}

Argon::Parser::Parser(Parser&& parser) noexcept {
    m_options = std::move(parser.m_options);
}

Argon::Parser& Argon::Parser::operator=(const Parser& parser) {
    if (this != &parser) {
        // Cleanup existing object
        for (auto& option : parser.m_options) {
            delete option;
        }
        m_options.clear();

        // Copy from old object 
        m_options.reserve(parser.m_options.size());
        for (auto& option : parser.m_options) {
            m_options.push_back(option->clone());
        }
    }
    return *this;
}

Argon::Parser& Argon::Parser::operator=(Parser&& parser) noexcept {
    if (this != &parser) {
        // Cleanup existing object
        for (auto& option : m_options) {
            delete option;
        }
        // Move from old object
        m_options = std::move(parser.m_options);
    }
    return *this;
}

Argon::Parser::~Parser() {
    for (auto& option : m_options) {
        delete option;
    }
}

void Argon::Parser::addOption(const IOption& option) {
    m_options.push_back(option.clone());
 }

Argon::IOption *Argon::Parser::getOption(const std::string& flag) {
    return Argon::getOption(m_options, flag);
}

Argon::IOption* Argon::getOption(const std::vector<IOption*>& options, const std::string& flag) {
    auto it = std::ranges::find_if(options, [&flag](const auto& option) {
       return std::ranges::contains(option->get_flags(), flag); 
    });
    return it == options.end() ? nullptr : *it;
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
    ast.analyze(*this, m_options);
    m_syntaxErrors.printErrorsTreeMode();
    m_analysisErrors.printErrorsTreeMode();
    // m_analysisErrors.printErrorsFlatMode();
}

Argon::StatementAst Argon::Parser::parseStatement() {
    StatementAst statement;
    std::vector<std::string> localFlags = getLocalFlags(m_options);
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
            statement.addOption(parseOptionGroup(m_options, localFlags));
        }
        // Option
        else if (token2.kind == TokenKind::IDENTIFIER) {
            m_scanner.rewind();
            statement.addOption(parseOption(localFlags));
        }
        
        stop = m_scanner.seeTokenKind(TokenKind::END);
    }
    return statement;
}

std::unique_ptr<Argon::OptionAst> Argon::Parser::parseOption(const std::vector<std::string>& sameLevelFlags) {
    bool error = false;
    // Get flag
    Token flag = m_scanner.getNextToken();
    if (flag.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position), flag.position);
        flag = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

    // If the flag is not valid
    if (!std::ranges::contains(sameLevelFlags, flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);
        error = true;
    }
    
    // Get value
    m_scanner.recordPosition();
    Token value = m_scanner.getNextToken();
    if (value.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag value, got '{}' at position {}", value.image, value.position), value.position);
        value = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

    // If value matches a flag
    if (std::ranges::contains(sameLevelFlags, value.image)) {
        m_syntaxErrors.addErrorMessage(std::format("No value provided for flag '{}' at position {}", flag.image, flag.position), value.position);
        m_scanner.rewind();
        error = true;
    }
    
    return error ? nullptr : std::make_unique<OptionAst>(flag.image, value.image, flag.position, value.position);
}

std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup(
    const std::vector<IOption*>& sameLevelOptions, const std::vector<std::string>& sameLevelFlags) {
    // Get flag
    Token flag = m_scanner.getNextToken();
    if (flag.kind != TokenKind::IDENTIFIER) {
        m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", flag.image, flag.position), flag.position);
        flag = m_scanner.scanUntilGet({TokenKind::IDENTIFIER});
    }

    bool validFlag = true;
    // If the flag is not valid
    if (!std::ranges::contains(sameLevelFlags, flag.image)) {
        m_analysisErrors.addErrorMessage(std::format("Unknown flag: '{}' at position {}", flag.image, flag.position), flag.position);
        validFlag = false;
    }

    OptionGroup *groupOption = dynamic_cast<OptionGroup*>(Argon::getOption(sameLevelOptions, flag.image));
    if (groupOption == nullptr && validFlag) {
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
        Token end = m_scanner.scanUntilGet({TokenKind::RBRACK});
        if (end.kind == TokenKind::END) {
            m_syntaxErrors.addErrorMessage("No matching ']' found for flag " + flag.image, end.position);
            m_syntaxErrors.addErrorMessage(std::format("No matching ']' found for flag {} at position {}", flag.image, flag.position), end.position);
        }
        return nullptr;
    }
    
    std::unique_ptr<OptionGroupAst> optionGroup = std::make_unique<OptionGroupAst>(flag.image, flag.position);
    
    // TODO: Right now, this can immediately stop if there is '[]' with no options inside
    while (true) {
        std::vector<std::string> nextLevelFlags = getLocalFlags(groupOption->get_options());
        
        // Get flag
        m_scanner.recordPosition();
        Token token1 = m_scanner.getNextToken();

        if (token1.kind != TokenKind::RBRACK && token1.kind != TokenKind::IDENTIFIER) {
            m_syntaxErrors.addErrorMessage(std::format("Expected flag name, got '{}' at position {}", token1.image, token1.position), token1.position);
            m_scanner.scanUntilGet({TokenKind::IDENTIFIER, TokenKind::LBRACK});
        }
        
        if (token1.kind == TokenKind::RBRACK) {
            optionGroup->endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroup->flag, optionGroup->flagPos, optionGroup->endPos);
            return optionGroup;
        } else if (token1.kind == TokenKind::END) {
            optionGroup->endPos = token1.position;
            m_analysisErrors.addErrorGroup(optionGroup->flag, optionGroup->flagPos, optionGroup->endPos);
            m_syntaxErrors.addErrorMessage("No matching ']' found for group " + flag.image, token1.position);
            return optionGroup;
        } 

        // Get value for option/lbrack for option group
        Token token2 = m_scanner.getNextToken();

        if (token2.kind != TokenKind::LBRACK && token2.kind != TokenKind::IDENTIFIER) {
            m_syntaxErrors.addErrorMessage(std::format("Expected option value or '[', got {} at position {}", token2.image, token2.position), token2.position);
            m_scanner.scanUntilGet({TokenKind::IDENTIFIER, TokenKind::LBRACK});
        }
        
        if (token2.kind == TokenKind::IDENTIFIER) {
            m_scanner.rewind();
            optionGroup->addOption(parseOption(nextLevelFlags));
        } else if (token2.kind == TokenKind::LBRACK) {
            m_scanner.rewind();
            optionGroup->addOption(parseOptionGroup(groupOption->get_options(), nextLevelFlags));
        } else if (token2.kind == TokenKind::END) {
            m_syntaxErrors.addErrorMessage(
                    std::format("Expected option or option group, got {} at position {}", token2.image, token2.position),
                    token2.position);
            m_syntaxErrors.addErrorMessage("No matching ']' found for group " + flag.image, token1.position);
            return optionGroup;
        }
    }
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}
