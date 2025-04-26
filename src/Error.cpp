#include "Argon/Error.hpp"

#include <iostream>

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

void Argon::ErrorGroup::printErrors() const {
    printErrors("");
}

void Argon::ErrorGroup::useTestPrint() const {
    chat();
    return;
    // std::vector<StringGroup> groups;
    // testPrint(groups, "", true);
    // int groupsSize = static_cast<int>(groups.size());
    // for (int i = 0; i < groupsSize; i++) {
    //     for (const auto& msg : groups[i]) {
    //         std::cout << msg << "\n";
    //     }
    //     if (i < groupsSize - 1) {
    //         std::cout << "\n";
    //     }
    // }
}

void Argon::ErrorGroup::printErrors(const std::string& groupSoFar) const {
    std::string newGroupName = groupSoFar + " > " + m_groupName;

    bool printedGroupName = false;
    
    for (auto& error : m_errors) {
        if (std::holds_alternative<ErrorMessage>(error)) {
            if (!printedGroupName) {
                printedGroupName = true;
                std::cout << "[" << newGroupName << "]" << '\n';
            }
            std::cout << " - " << std::get<ErrorMessage>(error).msg << "\n";
        }
        if (std::holds_alternative<ErrorGroup>(error)) {
            const ErrorGroup& group = std::get<ErrorGroup>(error);
            group.printErrors(newGroupName);
            printedGroupName = false;
        }
    }
}

// void Argon::ErrorGroup::printErrors(const std::string& groupSoFar) const {
//     std::string newGroupName = groupSoFar + " > " + m_groupName;
//
//     bool printedGroupName = false;
//     
//     for (auto& error : m_errors) {
//         if (std::holds_alternative<ErrorMessage>(error)) {
//             if (!printedGroupName) {
//                 printedGroupName = true;
//                 std::cout << "[" << newGroupName << "]" << '\n';
//             }
//             std::cout << "- " << std::get<ErrorMessage>(error).msg << "\n";
//         }
//         if (std::holds_alternative<ErrorGroup>(error)) {
//             const ErrorGroup& group = std::get<ErrorGroup>(error);
//             group.printErrors(newGroupName);
//             printedGroupName = false;
//         }
//     }
// }

void Argon::ErrorGroup::testPrint(std::vector<StringGroup>& groups, const std::string& groupNames, bool isRoot) const {
    std::string newGroupName = groupNames + " > " + m_groupName;

    bool printedGroupName = false;
    StringGroup stringGroup;
    
    for (auto& error : m_errors) {
        if (std::holds_alternative<ErrorMessage>(error)) {
            if (!printedGroupName) {
                printedGroupName = true;
                stringGroup.groups.emplace_back("[" + newGroupName + "]");
            }
            stringGroup.errors.emplace_back(" - " + std::get<ErrorMessage>(error).msg);
        }
        if (std::holds_alternative<ErrorGroup>(error)) {
            if (!stringGroup.errors.empty()) {
                groups.push_back(stringGroup);
                stringGroup.errors.clear();
            }
            const ErrorGroup& group = std::get<ErrorGroup>(error);
            group.testPrint(groups, newGroupName, false);
            printedGroupName = false;
        }
    }

    if (!stringGroup.errors.empty()) {
        groups.push_back(stringGroup);
    }
}

void Argon::ErrorGroup::chat() const {
    auto printRecursive = [](const ErrorGroup& group, const std::string& parentGroups, const bool isRootGroup, bool isFirstPrint, auto&& printRecursiveRef) -> void {
        std::string currentGroupPath;
        if (isRootGroup) {
            currentGroupPath = group.getGroupName();
        } else {
            currentGroupPath = parentGroups.empty() ? group.getGroupName() : parentGroups + " > " + group.getGroupName();
        }

        std::vector<std::string> messages;
        
        for (const auto& error : group.getErrors()) {
            std::visit([&](const auto& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, ErrorMessage>) {
                    messages.emplace_back(e.msg);
                } else if constexpr (std::is_same_v<T, ErrorGroup>) {
                    if (!messages.empty()) {
                        if (!isFirstPrint) {
                            std::cout << "\n";
                        }
                        std::cout << "[" << currentGroupPath << "]\n";
                        for (const auto& msg : messages) {
                            std::cout << " - " << msg << "\n";
                        }
                        messages.clear();
                        isFirstPrint = false;
                    }
                    printRecursiveRef(e, isRootGroup ? "" : currentGroupPath, false, isFirstPrint, printRecursiveRef);
                }
            }, error);
        }
        if (!messages.empty()) {
            if (!isFirstPrint) {
                std::cout << "\n";
            }
            std::cout << "[" << currentGroupPath << "]\n";
            for (const auto& msg : messages) {
                std::cout << " - " << msg << "\n";
            }
        }
    };

    printRecursive(*this, "", true, true, printRecursive);
}