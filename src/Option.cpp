#include "Option.hpp"

#include <iostream>

Argon::IOption& Argon::IOption::operator[](const std::string& tag) {
    m_flags.push_back(tag);
    return *this;
}

Argon::Parser Argon::IOption::operator|(const IOption& other) {
    Parser parser;
    parser.addOption(*this);
    parser.addOption(other);
    return parser;
}

const std::vector<std::string>& Argon::IOption::get_flags() const {
    return m_flags;
}

void Argon::IOption::print_flags() const {
    for (auto& tag : m_flags) {
        std::cout << tag << "\n";
    }
}

const std::string& Argon::IOption::get_error() const {
    return m_error;
}

bool Argon::IOption::has_error() const {
    return !m_error.empty();
}
