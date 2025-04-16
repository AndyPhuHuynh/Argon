#include "Argon/Parser.hpp"

#include <algorithm>
#include <iostream>

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
    return getOption(m_options, flag);
}

Argon::IOption* Argon::Parser::getOption(const std::vector<IOption*>& options, const std::string& flag) {
    auto it = std::ranges::find_if(options, [&flag](const auto& option) {
       return std::ranges::contains(option->get_flags(), flag); 
    });
    return it == options.end() ? nullptr : *it;
}

void Argon::Parser::addError(const std::string& error, int pos) {
    if (m_groupParseStack.empty()) {
        m_errors.emplace(error, pos);
        return;
    }
    
    std::string tabs;
    auto& [flag, used] = m_groupParseStack.back();
    if (!used) {
        for (auto& [prevFlag, prevUsed] : m_groupParseStack) {
            if (!prevUsed) {
                prevUsed = true;
                std::string err = tabs;
                err.append("Error parsing group '")
                   .append(prevFlag)
                   .append("':");
                m_errors.emplace(std::move(err), pos);       
            }
            tabs += "\t";
        }
    }
    m_errors.emplace(tabs + error, pos);
}

void Argon::Parser::addGroupToParseStack(const std::string& flag) {
    m_groupParseStack.emplace_back(flag, false);
}

void Argon::Parser::popGroupParseStack() {
    if (!m_groupParseStack.empty()) {
        m_groupParseStack.pop_back();
    }
}

void Argon::Parser::parseString(const std::string& str) {
    m_scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(*this, m_options);

    if (!m_errors.empty()) {
        for (auto& error : m_errors) {
            std::cout << error.msg << ", at column: " << error.pos << std::endl;
        }
    }
}

Argon::StatementAst Argon::Parser::parseStatement() {
    StatementAst statement;
    bool stop = false;
    while (!stop) {
        m_scanner.recordPosition();
        Token token1 = m_scanner.getNextToken();
        Token token2 = m_scanner.getNextToken();
        m_scanner.rewind();

        if (token1.kind != TokenKind::IDENTIFIER) {
            std::cerr << "Identifier expected!\n";
        } 
        
        // OptionGroup
        if (token2.kind == TokenKind::LBRACK) {
            statement.addOption(parseOptionGroup());
        }
        // Option
        else if (token2.kind == TokenKind::IDENTIFIER) {
            statement.addOption(parseOption());
        } 
        // Error
        else {
            std::cerr << "Error parsing statement, not an option or option group!\n";
        }
        stop = m_scanner.seeTokenKind(TokenKind::END) || m_scanner.seeTokenKind(TokenKind::RBRACK);
    }
    return statement;
}

std::unique_ptr<Argon::OptionAst> Argon::Parser::parseOption() {
    Token tag = m_scanner.getNextToken();
    Token value = m_scanner.getNextToken();

    if (tag.kind != TokenKind::IDENTIFIER) {
        std::cerr << "Error: Expected an identifier token for tag\n";
    }
    if (value.kind != TokenKind::IDENTIFIER) {
        std::cerr << "Error: Expected an identifier token for value\n";
    }

    return std::make_unique<OptionAst>(tag.image, value.image, tag.position, value.position);
    // TODO: check if value is identifier
}

std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup() {
    Token tag = m_scanner.getNextToken();
    Token lbrack = m_scanner.getNextToken();

    std::unique_ptr<OptionGroupAst> optionGroup = std::make_unique<OptionGroupAst>(tag.image, tag.position);
    
    // TODO: Right now, this can immediately stop if there is '[]' with no options inside
    while (true) {
        m_scanner.recordPosition();
        Token token1 = m_scanner.getNextToken();

        if (token1.kind == TokenKind::RBRACK) {
            break;
        } else if (token1.kind == TokenKind::END) {
            std::cerr << "Error RBRACK expected!\n";
            break;
        } else if (token1.kind != TokenKind::IDENTIFIER) {
            std::cerr << "Error: Expected an identifier token for tag\n";
        }

        Token token2 = m_scanner.getNextToken();
        if (token2.kind == TokenKind::IDENTIFIER) {
            m_scanner.rewind();
            optionGroup->addOption(parseOption());
        } else if (token2.kind == TokenKind::LBRACK) {
            m_scanner.rewind();
            optionGroup->addOption(parseOptionGroup());
        } else if (token2.kind == TokenKind::END) {
            std::cerr << "Error identifier or LBRACK expected!\n";
        }
    }

    return optionGroup;
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}
