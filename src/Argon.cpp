#include "Argon/Argon.hpp"

#include <algorithm>
#include <iostream>

#include "Argon/Option.hpp"
#include "Argon/StringUtil.hpp"

Argon::Parser::Parser(const Parser& parser) {
    for (auto& option : parser.options) {
        this->options.push_back(option->clone());
    }
}

Argon::Parser::Parser(Parser&& parser) noexcept {
    options = std::move(parser.options);
}

Argon::Parser& Argon::Parser::operator=(const Parser& parser) {
    if (this != &parser) {
        // Cleanup existing object
        for (auto& option : parser.options) {
            delete option;
        }
        options.clear();

        // Copy from old object 
        options.reserve(parser.options.size());
        for (auto& option : parser.options) {
            options.push_back(option->clone());
        }
    }
    return *this;
}

Argon::Parser& Argon::Parser::operator=(Parser&& parser) noexcept {
    if (this != &parser) {
        // Cleanup existing object
        for (auto& option : options) {
            delete option;
        }
        // Move from old object
        options = std::move(parser.options);
    }
    return *this;
}

Argon::Parser::~Parser() {
    for (auto& option : options) {
        delete option;
    }
}

void Argon::Parser::addOption(const IOption& option) {
    options.push_back(option.clone());
 }

Argon::IOption *Argon::Parser::getOption(const std::string& flag) {
    return getOption(options, flag);
}

Argon::IOption* Argon::Parser::getOption(const std::vector<IOption*>& options, const std::string& flag) {
    auto it = std::ranges::find_if(options, [&flag](const auto& option) {
       return std::ranges::contains(option->get_flags(), flag); 
    });
    return it == options.end() ? nullptr : *it;
}

void Argon::Parser::parseString(const std::string& str) {
    scanner = Scanner(str);
    StatementAst ast = parseStatement();
    ast.analyze(options);
}

Argon::StatementAst Argon::Parser::parseStatement() {
    StatementAst statement;
    bool stop = false;
    while (!stop) {
        scanner.recordPosition();
        Token token1 = scanner.getNextToken();
        Token token2 = scanner.getNextToken();
        scanner.rewind();

        if (token1.kind != IDENTIFIER) {
            std::cerr << "Identifier expected!\n";
        } 
        
        // OptionGroup
        if (token2.kind == LBRACK) {
            statement.addOption(parseOptionGroup());
        }
        // Option
        else if (token2.kind == IDENTIFIER) {
            statement.addOption(parseOption());
        } 
        // Error
        else {
            std::cerr << "Error!\n";
        }
        stop = scanner.seeTokenKind(END) || scanner.seeTokenKind(RBRACK);
    }
    return statement;
}

std::unique_ptr<Argon::OptionAst> Argon::Parser::parseOption() {
    Token tag = scanner.getNextToken();
    Token value = scanner.getNextToken();

    if (tag.kind != IDENTIFIER) {
        std::cerr << "Error: Expected an identifier token for tag\n";
    }
    if (value.kind != IDENTIFIER) {
        std::cerr << "Error: Expected an identifier token for value\n";
    }

    return std::make_unique<OptionAst>(tag.image, value.image);
    // TODO: check if value is identifier
}

std::unique_ptr<Argon::OptionGroupAst> Argon::Parser::parseOptionGroup() {
    Token tag = scanner.getNextToken();
    Token lbrack = scanner.getNextToken();

    std::unique_ptr<OptionGroupAst> optionGroup = std::make_unique<OptionGroupAst>(tag.image);
    
    // TODO: Right now, this can immediately stop if there is '[]' with no options inside
    while (true) {
        scanner.recordPosition();
        Token token1 = scanner.getNextToken();

        if (token1.kind == RBRACK) {
            break;
        } else if (token1.kind == END) {
            std::cerr << "Error RBRACK expected!\n";
            break;
        } else if (token1.kind != IDENTIFIER) {
            std::cerr << "Error: Expected an identifier token for tag\n";
        }

        Token token2 = scanner.getNextToken();
        if (token2.kind == IDENTIFIER) {
            scanner.rewind();
            optionGroup->addOption(parseOption());
        } else if (token2.kind == RBRACK) {
            optionGroup->addOption(parseOptionGroup());
        } else if (token2.kind == END) {
            std::cerr << "Error identifier or LBRACK expected!\n";
        }
    }

    return optionGroup;
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}
