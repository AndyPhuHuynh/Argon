#include "Argon/Error.hpp"

#include <iostream>

Argon::ErrorMessage::ErrorMessage(std::string msg, const int pos) : msg(std::move(msg)), pos(pos) {}

std::strong_ordering Argon::ErrorMessage::operator<=>(const ErrorMessage& other) const {
    return pos <=> other.pos;
}

Argon::ErrorGroup::ErrorGroup(std::string groupName, const int startPos, const int endPos)
: m_groupName(std::move(groupName)), m_startPos(startPos), m_endPos(endPos) {
    
}

void Argon::ErrorGroup::addErrorMessage(const std::string& msg, int pos) {
    using ErrorVariant = std::variant<ErrorMessage, ErrorGroup>;
    
    if (m_errors.empty()) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos);
        return;
    }
    
    int index = 0;
    for (; index < static_cast<int>(m_errors.size()); index++) {
        ErrorVariant& item = m_errors[index];

        if (std::holds_alternative<ErrorMessage>(item)) {
            ErrorMessage& errorMsg = std::get<ErrorMessage>(item);
            if (errorMsg.pos > pos) {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(item)) {
            ErrorGroup& errorGroup = std::get<ErrorGroup>(item);
            if (errorGroup.m_startPos > pos) {
                break;
            }
        }
    }
    
    // Index is now the index of the item positioned ahead of where we want
    // If index is zero just insert it at 0
    if (index == 0) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos);
        return;
    }
    
    // Else check the previous index
    ErrorVariant& item = m_errors[index - 1];
    
    // If error group, check if it's within the bounds, if it is, insert into that group
    if (std::holds_alternative<ErrorGroup>(item)) {
        ErrorGroup& errorGroup = std::get<ErrorGroup>(item);

        if (pos > errorGroup.m_startPos && pos < errorGroup.m_endPos) {
            errorGroup.addErrorMessage(msg, pos);
            return;
        }
    }

    // Else insert at that index
    if (index >= static_cast<int>(m_errors.size())) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos);
    } else {
        m_errors.insert(m_errors.begin() + index, ErrorMessage(msg, pos));
    }
}

void Argon::ErrorGroup::addErrorGroup(const std::string& name, int startPos, int endPos) {
    using ErrorVariant = std::variant<ErrorMessage, ErrorGroup>;
    
    if (m_errors.empty()) {
        m_errors.emplace_back(std::in_place_type<ErrorGroup>, name, startPos, endPos);
        return;
    }
    
    int index = 0;
    for (; index < static_cast<int>(m_errors.size()); index++) {
        ErrorVariant& item = m_errors[index];

        if (std::holds_alternative<ErrorMessage>(item)) {
            ErrorMessage& errorMsg = std::get<ErrorMessage>(item);
            if (errorMsg.pos > startPos) {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(item)) {
            ErrorGroup& errorGroup = std::get<ErrorGroup>(item);
            if (errorGroup.m_startPos > startPos) {
                break;
            }
        }
    }
    // Index is now the index of the item positioned ahead of where we want
    // If index is zero just insert it at 0
    if (index == 0) {
        m_errors.emplace_back(std::in_place_type<ErrorGroup>, name, startPos, endPos);
        return;
    }
    
    // Else check the previous index
    ErrorVariant& item = m_errors[index - 1];
    
    // If error group, check if it's within the bounds, if it is, insert into that group
    if (std::holds_alternative<ErrorGroup>(item)) {
        ErrorGroup& errorGroup = std::get<ErrorGroup>(item);

        if (startPos > errorGroup.m_startPos && endPos < errorGroup.m_startPos) {
            errorGroup.addErrorGroup(name, startPos, endPos);
            return;
        } else if (startPos > errorGroup.m_startPos && startPos < errorGroup.m_endPos && endPos > errorGroup.m_endPos) {
            std::cerr << "Error adding error group, bounds overlap!\n";
        } else if (endPos > errorGroup.m_startPos && endPos < errorGroup.m_endPos && startPos < errorGroup.m_startPos) {
            std::cerr << "Error adding error group, bounds overlap!\n";
        }
    }

    // Else insert at that index
    if (index >= static_cast<int>(m_errors.size())) {
        m_errors.emplace_back(std::in_place_type<ErrorGroup>, name, startPos, endPos);
    } else {
        m_errors.insert(m_errors.begin() + index, ErrorGroup(name, startPos, endPos));
    }
}

const std::string& Argon::ErrorGroup::getGroupName() const {
    return m_groupName;
}

const std::vector<std::variant<Argon::ErrorMessage, Argon::ErrorGroup>>& Argon::ErrorGroup::getErrors() const {
    return m_errors;
}
