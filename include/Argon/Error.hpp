#ifndef ARGON_ERROR_INCLUDE
#define ARGON_ERROR_INCLUDE

#include <compare>
#include <string>
#include <variant>
#include <vector>

namespace Argon {
    struct ErrorMessage;
    class ErrorGroup;

    using ErrorVariant = std::variant<ErrorMessage, ErrorGroup>;

    enum class ErrorType {
        None = 0,

        Syntax_MissingFlagName,
        Syntax_MissingValue,
        Syntax_MissingLeftBracket,
        Syntax_MissingRightBracket,
        Syntax_UnknownFlag,
        Syntax_MisplacedPositional,

        Analysis_UnknownFlag,
        Analysis_IncorrectOptionType,
        Analysis_ConversionError,
        Analysis_UnexpectedToken
    };
    
    struct ErrorMessage {
        std::string msg;
        int pos = -1;
        ErrorType type = ErrorType::None;

        ErrorMessage() = default;
        ErrorMessage(std::string_view msg, int pos, ErrorType type);
        
        std::strong_ordering operator<=>(const ErrorMessage &other) const;
    };

    class ErrorGroup {
        std::string m_groupName;
        std::vector<ErrorVariant> m_errors;
        int m_startPos = -1;
        int m_endPos = -1;
        bool m_hasErrors = false;
        ErrorGroup *m_parent = nullptr;
    public:
        ErrorGroup() = default;
        ErrorGroup(std::string_view groupName, int startPos, int endPos);

        void clear();
        void addErrorMessage(std::string_view msg, int pos, ErrorType type);
        void addErrorGroup(std::string_view name, int startPos, int endPos);
        void removeErrorGroup(int startPos);

        [[nodiscard]] auto getGroupName() const -> const std::string&;
        [[nodiscard]] auto getErrors() const -> const std::vector<ErrorVariant>&;
        [[nodiscard]] auto hasErrors() const -> bool;
        [[nodiscard]] auto getStartPosition() const -> int;
        [[nodiscard]] auto getEndPosition() const -> int;

        void printErrors() const;
    private:
        ErrorGroup(std::string_view groupName, int startPos, int endPos, ErrorGroup* parent);

        void setHasErrors();
        void addErrorGroup(ErrorGroup& groupToAdd);
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <format>
#include <iostream>
#include <sstream>

static bool inRange(const int value, const int min, const int max) {
    return value >= min && value <= max;
}

inline Argon::ErrorMessage::ErrorMessage(const std::string_view msg, const int pos, const ErrorType type)
    : msg(msg), pos(pos), type(type) {}

inline std::strong_ordering Argon::ErrorMessage::operator<=>(const ErrorMessage& other) const {
    return pos <=> other.pos;
}

inline Argon::ErrorGroup::ErrorGroup(const std::string_view groupName, const int startPos, const int endPos)
: m_groupName(groupName), m_startPos(startPos), m_endPos(endPos) {

}

inline void Argon::ErrorGroup::clear() {
    m_errors.clear();
    m_hasErrors = false;
}

inline Argon::ErrorGroup::ErrorGroup(const std::string_view groupName, const int startPos, const int endPos, ErrorGroup* parent)
: m_groupName(groupName), m_startPos(startPos), m_endPos(endPos), m_parent(parent) {

}

inline void Argon::ErrorGroup::setHasErrors() {
    auto group = this;
    while (group != nullptr) {
        group->m_hasErrors = true;
        group = group->m_parent;
    }
}

inline void Argon::ErrorGroup::addErrorMessage(std::string_view msg, int pos, ErrorType type) { //NOLINT (recursion)
    if (m_errors.empty()) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos, type);
        setHasErrors();
        return;
    }

    size_t index = 0;
    for (; index < m_errors.size(); index++) {
        ErrorVariant& item = m_errors[index];

        if (std::holds_alternative<ErrorMessage>(item)) {
            const ErrorMessage& errorMsg = std::get<ErrorMessage>(item);
            if (errorMsg.pos > pos) {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(item)) {
            const ErrorGroup& errorGroup = std::get<ErrorGroup>(item);
            if (errorGroup.m_startPos > pos) {
                break;
            }
        }
    }

    // Index is now the index of the item positioned ahead of where we want
    // If index is zero just insert it at 0
    if (index == 0) {
        m_errors.emplace(m_errors.begin(), std::in_place_type<ErrorMessage>, msg, pos, type);
        setHasErrors();
        return;
    }

    // Else check the previous index
    ErrorVariant& item = m_errors[index - 1];

    // If error group, check if it's within the bounds, if it is, insert into that group
    if (std::holds_alternative<ErrorGroup>(item)) {
        auto& errorGroup = std::get<ErrorGroup>(item);

        if (inRange(pos, errorGroup.m_startPos, errorGroup.m_endPos)) {
            errorGroup.addErrorMessage(msg, pos, type);
            return;
        }
    }

    // Else insert at that index
    if (index >= m_errors.size()) {
        m_errors.emplace_back(std::in_place_type<ErrorMessage>, msg, pos, type);
        setHasErrors();
    } else {
        m_errors.emplace(m_errors.begin() + static_cast<std::ptrdiff_t>(index), std::in_place_type<ErrorMessage>,
            msg, pos, type);
        setHasErrors();
    }
}

inline void Argon::ErrorGroup::addErrorGroup(const std::string_view name, const int startPos, const int endPos) {
    auto groupToAdd = ErrorGroup(name, startPos, endPos, this);
    addErrorGroup(groupToAdd);
}

inline void Argon::ErrorGroup::addErrorGroup(ErrorGroup& groupToAdd) { //NOLINT (recursion)
    if (m_errors.empty()) {
        m_errors.emplace_back(std::move(groupToAdd));
        return;
    }

    // Find the sorted index of the start position
    size_t insertIndex = 0;
    for (; insertIndex < m_errors.size(); insertIndex++) {
        ErrorVariant& item = m_errors[insertIndex];

        if (std::holds_alternative<ErrorMessage>(item)) {
            const ErrorMessage& errorMsg = std::get<ErrorMessage>(item);
            if (errorMsg.pos > groupToAdd.m_startPos) {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(item)) {
            const ErrorGroup& errorGroup = std::get<ErrorGroup>(item);
            if (errorGroup.m_startPos > groupToAdd.m_startPos) {
                break;
            }
        }
    }

    if (insertIndex != 0) {
        // Else check the previous index
        ErrorVariant& item = m_errors[insertIndex - 1];

        // If error group, check if it's within the bounds, if it is, insert into that group
        if (std::holds_alternative<ErrorGroup>(item)) {
            auto& errorGroup = std::get<ErrorGroup>(item);

            const bool fullyInRange = inRange(groupToAdd.m_startPos, errorGroup.m_startPos, errorGroup.m_endPos) &&
                                      inRange(groupToAdd.m_endPos, errorGroup.m_startPos, errorGroup.m_endPos);
            if (fullyInRange) {
                errorGroup.addErrorGroup(groupToAdd);
                return;
            }
            const bool fullyAfter = groupToAdd.m_startPos >= errorGroup.m_endPos &&
                                    groupToAdd.m_endPos >= errorGroup.m_endPos;
            if (!fullyAfter) {
                std::cerr << "Internal Argon Error: Adding error group, not fully in range!\n";
            }
        }
    }

    // If the previous item was not a group OR if it was a group and the error we want to add is not in its bounds
    // Check for every item after index and insert it into the new group, if its fully within bounds
    while (insertIndex < m_errors.size()) {
        if (std::holds_alternative<ErrorMessage>(m_errors[insertIndex])) {
            auto& errorMsg = std::get<ErrorMessage>(m_errors[insertIndex]);
            if (inRange(errorMsg.pos, groupToAdd.m_startPos, groupToAdd.m_endPos)) {
                groupToAdd.addErrorMessage(errorMsg.msg, errorMsg.pos, errorMsg.type);
                m_errors.erase(m_errors.begin() + static_cast<std::ptrdiff_t>(insertIndex));
            } else {
                break;
            }
        } else if (std::holds_alternative<ErrorGroup>(m_errors[insertIndex])) {
            auto& errorGroup = std::get<ErrorGroup>(m_errors[insertIndex]);
            if (inRange(errorGroup.m_startPos, groupToAdd.m_startPos, groupToAdd.m_endPos) &&
                inRange(errorGroup.m_endPos, groupToAdd.m_startPos, groupToAdd.m_endPos)) {
                groupToAdd.addErrorGroup(errorGroup);
                m_errors.erase(m_errors.begin() + static_cast<std::ptrdiff_t>(insertIndex));
            } else if ((inRange(errorGroup.m_startPos, groupToAdd.m_startPos, groupToAdd.m_endPos) && !inRange(errorGroup.m_endPos, groupToAdd.m_startPos, groupToAdd.m_endPos)) ||
                (inRange(errorGroup.m_endPos, groupToAdd.m_startPos, groupToAdd.m_endPos) && !inRange(errorGroup.m_startPos, groupToAdd.m_startPos, groupToAdd.m_endPos))) {
                std::cerr << "Error adding error group, bounds overlap!\n";
            } else {
                break;
            }
        }
    }

    // Insert group at index
    if (insertIndex >= m_errors.size()) {
        m_errors.emplace_back(std::move(groupToAdd));
    } else {
        m_errors.insert(m_errors.begin() + static_cast<std::ptrdiff_t>(insertIndex), std::move(groupToAdd));
    }
}

inline void Argon::ErrorGroup::removeErrorGroup(int startPos) {
    const auto it = std::ranges::find_if(m_errors, [startPos](const ErrorVariant& item) {
        if (std::holds_alternative<ErrorGroup>(item)) {
            const auto& errorGroup = std::get<ErrorGroup>(item);
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

inline const std::string& Argon::ErrorGroup::getGroupName() const {
    return m_groupName;
}

inline const std::vector<std::variant<Argon::ErrorMessage, Argon::ErrorGroup>>& Argon::ErrorGroup::getErrors() const {
    return m_errors;
}

inline bool Argon::ErrorGroup::hasErrors() const {
    return m_hasErrors;
}

inline auto Argon::ErrorGroup::getStartPosition() const -> int {
    return m_startPos;
}

inline auto Argon::ErrorGroup::getEndPosition() const -> int {
    return m_endPos;
}

inline void Argon::ErrorGroup::printErrors() const {
    constexpr auto printRecursive = [](std::stringstream& stream, const ErrorGroup& group, const std::string& prefix,
                                       auto&& printRecursiveRef) -> void {
        const auto& errors = group.getErrors();
        for (size_t i = 0; i < errors.size(); ++i) {
            auto& error = errors[i];
            std::visit([&]<typename T>(const T& e) {
                if constexpr (std::is_same_v<T, ErrorMessage>) {
                    stream << std::format("{}{}\n", prefix, e.msg);
                } else if constexpr (std::is_same_v<T, ErrorGroup>) {
                    if (!e.m_hasErrors) {
                        return;
                    }
                    stream << "\n";
                    stream << std::format("{}In group '{}':\n", prefix, e.getGroupName());
                    printRecursiveRef(stream, e, prefix + "    ", printRecursiveRef);
                    if (i < errors.size() - 1) {
                        stream << "\n";
                    }
                }
            }, error);
        }
    };

    if (!m_hasErrors) {
        return;
    }

    std::stringstream ss;
    printRecursive(ss, *this, "", printRecursive);
    std::cout << ss.str();
}

#endif // ARGON_ERROR_INCLUDE