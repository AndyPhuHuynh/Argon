#include "Argon.hpp"

#include <iostream>

#include "Option.hpp"

/**
 * @brief Tokenizes a string based on a delimiter
 * @param str The string that will be tokenized
 * @param delimiter The character used as the delimiter
 * @return A vector of the tokens in the tokenized string
 */
static std::vector<std::string> SplitString(const std::string& str, const char delimiter) {
    std::vector<std::string> tokens;
    int prevIndex = 0;
    const int length = static_cast<int>(str.length());
    for (int i = 0; i < length; i++) {
        if (str[i] == delimiter) {
            // Add token only if it's non-empty
            if (prevIndex != i) {
                tokens.push_back(str.substr(prevIndex, i - prevIndex));
            }
            // Skip consecutive delimiters
            while (i < length && str[i] == delimiter) {
                i++;
            }
            prevIndex = i;
        }
    }

    // Add the last token if its non-empty
    if (prevIndex < length) {
        tokens.push_back(str.substr(prevIndex));
    }
    
    return tokens;
}

void Argon::Parser::addOption(const IOption& option) {
    options.push_back(option.clone());
}

void Argon::Parser::addOption(const std::shared_ptr<IOption>& option) {
    options.push_back(option);
}

bool Argon::Parser::tokenInOptions(const std::string& token) {
    for (auto& option : options) {
        for (const auto& tag : option->tags) {
            if (token == tag) {
                return true;
            }
        }
    }
    return false;
}

Argon::IOption& Argon::Parser::searchOptions(const std::string& token) {
    for (auto& option : options) {
        for (const auto& tag : option->tags) {
            if (token == tag) {
                return *option;
            }
        }
    }
    std::cerr << token << " not found\n";
    // TODO: FIX
    exit(1);
}


void Argon::Parser::parseString(const std::string& str) {
    std::vector<std::string> tokens = SplitString(str, ' ');

    if (tokens.empty()) {
        return;
    }
    
    size_t tokenIndex = 0;
    while (tokenIndex < tokens.size()) {
        std::string& token = tokens[tokenIndex++];
        if (!tokenInOptions(token)) {
            continue;   
        }
        IOption& option = searchOptions(token);
        std::string& nextToken = tokens[tokenIndex++];
        option.setValue(nextToken);
    }
}

Argon::Parser& Argon::Parser::operator|(const IOption& option) {
    addOption(option);
    return *this;
}