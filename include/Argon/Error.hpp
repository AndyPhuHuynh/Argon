#pragma once

#include <compare>
#include <string>
#include <variant>
#include <vector>

namespace Argon {
    struct ErrorMessage;
    class ErrorGroup;

    using ErrorVariant = std::variant<ErrorMessage, ErrorGroup>;
    struct StringGroup {
        std::vector<std::string> groups;
        std::vector<std::string> errors;
    };
    
    struct ErrorMessage {
        std::string msg;
        int pos;

        ErrorMessage() = default;
        ErrorMessage(std::string msg, int pos);
        
        std::strong_ordering operator<=>(const ErrorMessage &other) const;
    };

    class ErrorGroup {
        std::string m_groupName;
        std::vector<ErrorVariant> m_errors;
        int m_startPos;
        int m_endPos;
    public:
        ErrorGroup() = default;
        ErrorGroup(std::string groupName, int startPos, int endPos);

        void addErrorMessage(const std::string& msg, int pos);
        void addErrorGroup(const std::string& name, int startPos, int endPos);

        const std::string& getGroupName() const;
        const std::vector<ErrorVariant>& getErrors() const;

        void printErrors() const;
        void useTestPrint() const;
    private:
        void printErrors(const std::string& groupSoFar) const;
        void testPrint(std::vector<StringGroup>& groups, const std::string& groupNames, bool isRoot) const;

        void chat() const;
    };
}
