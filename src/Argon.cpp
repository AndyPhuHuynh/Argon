#include "Argon.hpp"

#include <iostream>

#include "Option.hpp"
#include <StringUtil.h>

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
        for (auto& option : parser.options) {
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

bool Argon::Parser::getOption(const std::string& token, IOption*& out) {
    for (auto& option : options) {
        for (const auto& tag : option->get_flags()) {
            if (token == tag) {
                out = option;
                return true;
            }
        }
    }
    return false;
}

void Argon::Parser::parseString(const std::string& str) {
    std::vector<std::string> tokens = StringUtil::split_string(str, ' ');

    if (tokens.empty()) {
        return;
    }
    
    size_t tokenIndex = 0;
    while (tokenIndex < tokens.size()) {
        std::string& token = tokens[tokenIndex++];

        IOption *option = nullptr;
        if (!getOption(token, option)) {
            continue;
        }
        
        std::string& nextToken = tokens[tokenIndex++];
        option->set_value(token, nextToken);
    }

    for (auto& option : options) {
        if (!option->has_error()) {
            return;
        }

        std::cout << option->get_error() << "\n";
    }
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}