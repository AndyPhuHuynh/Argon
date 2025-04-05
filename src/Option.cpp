#include "Option.hpp"

Argon::IOption& Argon::IOption::operator[](const std::string& tag) {
    tags.push_back(tag);
    return *this;
}

Argon::Parser Argon::IOption::operator|(const IOption& other) {
    Parser parser;
    parser.addOption(*this);
    parser.addOption(other);
    return parser;
}

void Argon::IOption::printTags() const {
    for (auto& tag : tags) {
        std::cout << tag << "\n";
    }
}