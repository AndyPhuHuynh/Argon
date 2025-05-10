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
        int m_startPos = -1;
        int m_endPos = -1;
        bool m_hasErrors = false;
        ErrorGroup *m_parent = nullptr;
    public:
        ErrorGroup() = default;
        ErrorGroup(std::string groupName, int startPos, int endPos);
        ErrorGroup(std::string groupName, int startPos, int endPos, ErrorGroup* parent);

        void setHasErrors();
        
        void addErrorMessage(const std::string& msg, int pos);
        void addErrorGroup(const std::string& name, int startPos, int endPos);
        void removeErrorGroup(int startPos);

        const std::string& getGroupName() const;
        const std::vector<ErrorVariant>& getErrors() const;
        
        void printErrorsFlatMode() const;
        void printErrorsTreeMode() const;
    private:
        void addErrorGroup(ErrorGroup& groupToAdd);
        size_t getIndexOfLastHasError () const; 
    };
}
