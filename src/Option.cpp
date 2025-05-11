#include "Argon/Option.hpp"

#include <algorithm>
#include <iostream>

Argon::IOption::IOption(const IOption& other) {
    m_flags = other.m_flags;
    m_error = other.m_error;
}

Argon::IOption& Argon::IOption::operator=(const IOption& other) {
    if (this != &other) {
        m_flags = other.m_flags;
        m_error = other.m_error;
    }
    return *this;
}

Argon::IOption::IOption(IOption&& other) noexcept {
    if (this != &other) {
        m_flags = std::move(other.m_flags);
        m_error = std::move(other.m_error);
    }
}

Argon::IOption& Argon::IOption::operator=(IOption&& other) noexcept {
    if (this != &other) {
        m_flags = std::move(other.m_flags);
        m_error = std::move(other.m_error);
    }
    return *this;
}

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

Argon::IOption::operator Argon::Parser() const {
    Parser parser;
    parser.addOption(*this);
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

void Argon::IOption::clear_error() {
    m_error.clear();
}

Argon::OptionGroup& Argon::OptionGroup::operator+(const IOption& other) {
    add_option(other);
    return *this;
}

void Argon::OptionGroup::add_option(const IOption& option) {
    m_context.addOption(option);
}

Argon::IOption *Argon::OptionGroup::get_option(const std::string& flag) {
    return m_context.getOption(flag);
}

Argon::Context& Argon::OptionGroup::get_context() {
    return m_context;
}

bool Argon::parseBool(const std::string& arg, bool& out) {
    std::string boolStr = arg;
    StringUtil::to_lower(boolStr);
    if (boolStr == "true") {
        out = true;
        return true;
    } else if (boolStr == "false") {
        out = false;
        return true;
    }
    return false;
}
