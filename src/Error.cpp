#include "Argon/Error.hpp"

#include <format>
#include <iostream>
#include <sstream>

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

Argon::ErrorGroup::ErrorGroup(std::string groupName, const int startPos, const int endPos, ErrorGroup* parent)
: m_groupName(std::move(groupName)), m_startPos(startPos), m_endPos(endPos), m_parent(parent) {
    
}

void Argon::ErrorGroup::setHasErrors() {
    ErrorGroup *group = this;
    while (group != nullptr) {
        group->m_hasErrors = true;
        group = group->m_parent;
    }
}

void Argon::ErrorGroup::addErrorMessage(const std::string& msg, int pos) {
    if (m_errors.empty()) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos);
        setHasErrors();
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
        setHasErrors();
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
        setHasErrors();
    } else {
        m_errors.emplace(m_errors.begin() + index, std::in_place_type<ErrorMessage>, msg, pos);
        setHasErrors();
    }
}

void Argon::ErrorGroup::addErrorGroup(const std::string& name, int startPos, int endPos) {
    if (m_errors.empty()) {
        m_errors.emplace_back(std::in_place_type<ErrorGroup>, name, startPos, endPos, this);
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
    ErrorGroup groupToAdd(name, startPos, endPos, this);
    
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

void Argon::ErrorGroup::removeErrorGroup(int startPos) {
    auto it = std::ranges::find_if(m_errors, [startPos](const ErrorVariant& item) {
        if (std::holds_alternative<ErrorGroup>(item)) {
            const ErrorGroup& errorGroup = std::get<ErrorGroup>(item);
            if (errorGroup.m_startPos == startPos) {
                return true;
            }
        }
        return false;
    });

    if (it != m_errors.end()) {
        m_errors.erase(it);
    }
}

const std::string& Argon::ErrorGroup::getGroupName() const {
    return m_groupName;
}

const std::vector<std::variant<Argon::ErrorMessage, Argon::ErrorGroup>>& Argon::ErrorGroup::getErrors() const {
    return m_errors;
}

void Argon::ErrorGroup::printErrorsFlatMode() const {
    constexpr auto printRecursive = [](std::stringstream& stream, const ErrorGroup& group,
                                       const std::string& parentGroups, const bool isRootGroup, bool isFirstPrint,
                                       auto&& printRecursiveRef) -> void {
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
    std::cout << ss.str();
}

void Argon::ErrorGroup::printErrorsTreeMode() const {
    constexpr auto printRecursive = [](std::stringstream& stream, const ErrorGroup& group, const std::string& prefix,
                                       const bool isFirstPrint, auto&& printRecursiveRef) -> void {
        if (isFirstPrint) {
            stream << std::format("[{}]\n", group.getGroupName());
        }
        const auto& errors = group.getErrors();
        size_t lastErrorIndex = group.getIndexOfLastHasError();
        for (size_t i = 0; i < errors.size(); ++i) {
            auto& error = errors[i];
            std::visit([&]<typename T>(const T& e) {
                if constexpr (std::is_same_v<T, ErrorMessage>) {
                    if (i == lastErrorIndex) {
                        stream << std::format("{}└── {}\n", prefix, e.msg);
                    } else {    
                        stream << std::format("{}├── {}\n", prefix, e.msg);
                    }
                } else if constexpr (std::is_same_v<T, ErrorGroup>) {
                    if (!e.m_hasErrors) {
                        return;
                    }
                    
                    if (i == lastErrorIndex) {
                        stream << std::format("{}└── [{}]\n", prefix, e.getGroupName());
                        printRecursiveRef(stream, e, prefix + "    ", false, printRecursiveRef);
                    } else {
                        stream << std::format("{}├── [{}]\n", prefix, e.getGroupName());
                        printRecursiveRef(stream, e, prefix + "│   ", false, printRecursiveRef);
                    }
                }
            }, error);
        }
    };

    if (!m_hasErrors) {
        return;
    }
    
    std::stringstream ss;
    printRecursive(ss, *this, "", true, printRecursive);
    std::cout << ss.str();
}

size_t Argon::ErrorGroup::getIndexOfLastHasError() const {
    size_t index = m_errors.size();
    while (index-- > 0) {
        ErrorVariant errorVariant = m_errors[index];
        bool found = std::visit([]<typename T>(const T& e) -> bool {
            if constexpr (std::is_same_v<T, ErrorMessage>) {
                return true;
            } else {
                return e.m_hasErrors;
            }
        }, errorVariant);

        if (found) {
            return index;
        }
    }
    return 0;
}
