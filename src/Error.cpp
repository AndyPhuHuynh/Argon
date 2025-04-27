#include "Argon/Error.hpp"

#include <iostream>
#include <sstream>
#include <print>

static bool inRange(const int value, const int min, const int max) {
    return value >= min && value <= max;
}

Argon::ErrorMessage::ErrorMessage(std::string msg, const int pos) : msg(std::move(msg)), pos(pos) {}

std::strong_ordering Argon::ErrorMessage::operator<=>(const ErrorMessage& other) const {
    return pos <=> other.pos;
}

Argon::ErrorGroup::ErrorGroup(std::string groupName, const int startPos, const int endPos)
: m_groupName(std::move(groupName)), m_startPos(startPos), m_endPos(endPos) {
    
}

void Argon::ErrorGroup::addErrorMessage(const std::string& msg, int pos) {
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
        m_errors.emplace(m_errors.begin(), std::in_place_type<ErrorMessage>, msg, pos);
        return;
    }
    
    // Else check the previous index
    ErrorVariant& item = m_errors[index - 1];
    
    // If error group, check if it's within the bounds, if it is, insert into that group
    if (std::holds_alternative<ErrorGroup>(item)) {
        ErrorGroup& errorGroup = std::get<ErrorGroup>(item);

        if (inRange(pos, errorGroup.m_startPos, errorGroup.m_endPos)) {
            errorGroup.addErrorMessage(msg, pos);
            return;
        }
    }

    // Else insert at that index
    if (index >= static_cast<int>(m_errors.size())) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos);
    } else {
        m_errors.emplace(m_errors.begin() + index, std::in_place_type<ErrorMessage>, msg, pos);
    }
}

void Argon::ErrorGroup::addErrorGroup(const std::string& name, int startPos, int endPos) {
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
    if (index != 0) {
        // Else check the previous index
        ErrorVariant& item = m_errors[index - 1];
    
        // If error group, check if it's within the bounds, if it is, insert into that group
        if (std::holds_alternative<ErrorGroup>(item)) {
            ErrorGroup& errorGroup = std::get<ErrorGroup>(item);

            if (inRange(startPos, errorGroup.m_startPos, errorGroup.m_endPos)) {
                errorGroup.addErrorGroup(name, startPos, endPos);
            } else if ((inRange(startPos, errorGroup.m_startPos, errorGroup.m_endPos) && endPos > errorGroup.m_endPos) ||
                       (inRange(endPos, errorGroup.m_startPos, errorGroup.m_endPos) && startPos < errorGroup.m_startPos)) {
                std::cerr << "Error adding error group, bounds overlap!\n";
            }
        }
        return;
    }

    // If the previous item was not a group OR if it was a group and the error we want to add is not in its bounds
    // Check for every item after index and insert it into the new group, if its fully within bounds
    ErrorGroup groupToAdd(name, startPos, endPos);
    
    while (index < static_cast<int>(m_errors.size())) {
        if (std::holds_alternative<ErrorMessage>(m_errors[index])) {
            ErrorMessage& errorMsg = std::get<ErrorMessage>(m_errors[index]);
            if (inRange(errorMsg.pos, startPos, endPos)) {
                groupToAdd.addErrorMessage(errorMsg.msg, startPos);
                m_errors.erase(m_errors.begin() + index);
            } else {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(m_errors[index])) {
            ErrorGroup& errorGroup = std::get<ErrorGroup>(m_errors[index]);
            if (inRange(errorGroup.m_startPos, startPos, endPos) &&
                inRange(errorGroup.m_endPos, startPos, endPos)) {
                groupToAdd.addErrorGroup(errorGroup.m_groupName, errorGroup.m_startPos, errorGroup.m_endPos);
                m_errors.erase(m_errors.begin() + index);
            } else if ((inRange(errorGroup.m_startPos, startPos, endPos) && !inRange(errorGroup.m_endPos, startPos, endPos)) ||
                (inRange(errorGroup.m_endPos, startPos, endPos) && !inRange(errorGroup.m_startPos, startPos, endPos))) {
                std::cerr << "Error adding error group, bounds overlap!\n";
            } else {
                break;
            }
        }
    }
    
    // Insert group at index
    if (index >= static_cast<int>(m_errors.size())) {
        m_errors.emplace_back(std::move(groupToAdd));
    } else {
        m_errors.insert(m_errors.begin() + index, std::move(groupToAdd));
    }
}

const std::string& Argon::ErrorGroup::getGroupName() const {
    return m_groupName;
}

const std::vector<std::variant<Argon::ErrorMessage, Argon::ErrorGroup>>& Argon::ErrorGroup::getErrors() const {
    return m_errors;
}

void Argon::ErrorGroup::printErrorsFlatMode() const {
    constexpr auto printRecursive = [](std::stringstream& stream, const ErrorGroup& group, const std::string& parentGroups,
        const bool isRootGroup, bool isFirstPrint, auto&& printRecursiveRef) -> void {
        std::string currentGroupPath;
        if (isRootGroup) {
            currentGroupPath = group.getGroupName();
        } else {
            currentGroupPath = parentGroups.empty() ? group.getGroupName() : parentGroups + " > " + group.getGroupName();
        }

        std::vector<std::string> messages;

        // Loop through all errors
        for (const auto& error : group.getErrors()) {
            std::visit([&]<typename T>(const T& e) {
                // If it's an error message, add to messages
                if constexpr (std::is_same_v<T, ErrorMessage>) {
                    messages.emplace_back(e.msg);
                }
                // If it's a group recursively print
                else if constexpr (std::is_same_v<T, ErrorGroup>) {
                    // Print out messages in current group
                    if (!messages.empty()) {
                        if (!isFirstPrint) {
                            stream << "\n";
                        }
                        stream << std::format("[{}]\n", currentGroupPath);
                        for (const auto& msg : messages) {
                            stream << std::format(" - {}\n", msg);
                        }
                        messages.clear();
                        isFirstPrint = false;
                    }
                    // Recurse
                    printRecursiveRef(stream, e, isRootGroup ? "" : currentGroupPath, false, isFirstPrint, printRecursiveRef);
                }
            }, error);
        }
        // Print remaining messages
        if (!messages.empty()) {
            if (!isFirstPrint) {
                stream << "\n";
            }
            stream << std::format("[{}]\n", currentGroupPath);
            for (const auto& msg : messages) {
                stream << std::format(" - {}\n", msg);
            }
        }
    };

    std::stringstream ss;
    printRecursive(ss, *this, "", true, true, printRecursive);
    std::print("{}", ss.str());
}

void Argon::ErrorGroup::printErrorsTreeMode() const {
    constexpr auto printRecursive = [](std::stringstream& stream, const ErrorGroup& group, const std::string& prefix, auto&& printRecursiveRef) ->void {
        const auto& errors = group.getErrors();
        for (size_t i = 0; i < errors.size(); ++i) {
            auto& error = errors[i];
            std::visit([&]<typename T>(const T& e) {
                if constexpr (std::is_same_v<T, ErrorMessage>) {
                    if (i == errors.size() - 1) {
                        stream << std::format("{}└── {}\n", prefix, e.msg);
                    } else {
                        stream << std::format("{}├── {}\n", prefix, e.msg);
                    }
                } else if constexpr (std::is_same_v<T, ErrorGroup>) {
                    
                    if (i == errors.size() - 1) {
                        stream << std::format("{}└── [{}]\n", prefix, e.getGroupName());
                        printRecursiveRef(stream, e, prefix + "    ", printRecursiveRef);
                    } else {
                        stream << std::format("{}├── [{}]\n", prefix, e.getGroupName());
                        printRecursiveRef(stream, e, prefix + "│   ", printRecursiveRef);
                    }
                }
            }, error);
        }
    };
    std::stringstream ss;
    printRecursive(ss, *this, "", printRecursive);
    std::print("{}", ss.str());
}
