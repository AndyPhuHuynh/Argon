#ifndef ARGON_ERROR_INCLUDE
#define ARGON_ERROR_INCLUDE

#include <compare>
#include <string>
#include <vector>

namespace Argon {
    struct ErrorMessage;
    class ErrorGroup;

    struct ErrorMessage {
        std::string msg;
        int pos = -1;

        ErrorMessage() = default;
        ErrorMessage(std::string_view msg, int pos);
        
        std::strong_ordering operator<=>(const ErrorMessage &other) const;
    };

    class ErrorGroup {
        std::vector<ErrorMessage> m_errors;
    public:
        ErrorGroup() = default;

        void clear();
        void addErrorMessage(std::string_view msg, int pos);

        [[nodiscard]] const std::string& getGroupName() const;
        [[nodiscard]] const std::vector<ErrorMessage>& getErrors() const;
        [[nodiscard]] bool hasErrors() const;

        void printErrorsFlatMode() const;
    };
}

// --------------------------------------------- Implementations -------------------------------------------------------

#include <format>
#include <iostream>
#include <sstream>

inline Argon::ErrorMessage::ErrorMessage(const std::string_view msg, const int pos) : msg(msg), pos(pos) {}

inline std::strong_ordering Argon::ErrorMessage::operator<=>(const ErrorMessage& other) const {
    return pos <=> other.pos;
}

inline void Argon::ErrorGroup::clear() {
    m_errors.clear();
}

inline void Argon::ErrorGroup::addErrorMessage(std::string_view msg, int pos) { //NOLINT (recursion)
    const auto it = std::ranges::find_if(m_errors, [pos](const ErrorMessage& error) {
        return error.pos >= pos;
    });
    auto newError = ErrorMessage(msg, pos);
    m_errors.insert(it, std::move(newError));
}

inline const std::vector<Argon::ErrorMessage>& Argon::ErrorGroup::getErrors() const {
    return m_errors;
}

inline bool Argon::ErrorGroup::hasErrors() const {
    return !m_errors.empty();
}

inline void Argon::ErrorGroup::printErrorsFlatMode() const {
    std::stringstream ss;
    for (const auto& err : m_errors) {
        ss << err.msg << "\n";
    }
    std::cout << ss.str();
}

#endif // ARGON_ERROR_INCLUDE