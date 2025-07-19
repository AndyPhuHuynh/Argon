#ifndef ARGON_HELP_MESSAGE_INCLUDE
#define ARGON_HELP_MESSAGE_INCLUDE

#include <iomanip>
#include <string>
#include <sstream>

#include "Argon/Constraints.hpp"
#include "Argon/Context.hpp"

namespace Argon::detail {

class HelpMessage {
public:
    HelpMessage(const Context *context, const Constraints *constraints, size_t maxLineWidth);

    [[nodiscard]] auto get() const -> std::string;
private:
    static constexpr size_t m_maxFlagWidthBeforeWrapping = 32;
    static constexpr size_t m_nameIndent = 2;
    static constexpr size_t m_nextLevelIndent = 4;

    std::stringstream m_message;
    const size_t m_maxLineWidth;
    const Constraints *m_constraints;

    auto indent(size_t leadingSpaces);

    auto appendHeader(const Context *context) -> void;

    auto appendBody(const Context *context, size_t leadingSpaces, const FlagPath& groupPath) -> void;

    auto appendOptions(const Context *context, size_t leadingSpaces, const FlagPath& groupPath) -> bool;

    auto appendPositionals(const Context *context, size_t leadingSpaces, bool printNewLine, const FlagPath& groupPath) -> bool;

    auto appendGroups(const Context *context, size_t leadingSpaces, bool printNewLine, const FlagPath& groupPath) -> bool;

    auto appendName(size_t leadingSpaces, const IOption *option) -> size_t;

    auto appendInputHint(size_t leadingSpaces, size_t nameLength, const IOption *option) -> size_t;

    void appendDescription(size_t leadingSpaces, const IOption *option, size_t nameLen, size_t inputHintLen, FlagPath groupPath);

    auto appendIOption(size_t leadingSpaces, const IOption *option, const FlagPath& groupPath) -> void;
};

inline auto getOptionName(const IOption *option) -> std::string {
    if (const auto flag = dynamic_cast<const IFlag *>(option)) {
        return flag->getFlag().getString();
    }
    return option->getInputHint();
}

inline auto concatPositionalsToInputHint(std::stringstream& ss, const std::vector<OptionHolder<IOption>>& positionals) {
    if (positionals.empty()) return;
    ss << ' ';
    for (size_t i = 0; i < positionals.size(); ++i) {
        ss << positionals[i].getRef().getInputHint();
        if (i < positionals.size() - 1) {
            ss << ' ';
        }
    }
};

inline auto getBasicInputHint(const IOption *opt) -> std::string {
    if (const auto group = dynamic_cast<const OptionGroup *>(opt); group != nullptr) {
        if (const auto hint = group->getInputHint(); !hint.empty()) {
            return hint;
        }
        return std::format("[{}]", group->getFlag().mainFlag);
    }
    return opt->getInputHint();
};

inline auto getContextInputHint(const Context *context, const std::string_view nameOfOptions) -> std::string {
    std::stringstream ss;
    const auto& positionals = context->getPositionals();
    switch (context->getDefaultPositionalPolicy()) {
        case PositionalPolicy::BeforeFlags:
            concatPositionalsToInputHint(ss, positionals);
            ss << ' ' << nameOfOptions;
            break;
        case PositionalPolicy::Interleaved:
        case PositionalPolicy::AfterFlags:
            ss << ' ' << nameOfOptions;
            concatPositionalsToInputHint(ss, positionals);
            break;
        case PositionalPolicy::UseDefault:
            std::unreachable();
    };
    return ss.str();
}

// Get the input hint for an option. InputHint means the names of the types that the user has to input after the option
inline auto getFullInputHint(const IOption *option) -> std::string {
    if (const auto group = dynamic_cast<const OptionGroup *>(option); group != nullptr) {
        return getContextInputHint(&group->getContext(), getBasicInputHint(group));
    }
    if (dynamic_cast<const IFlag *>(option)) {
        return getBasicInputHint(option);
    }
    return "";
}

inline HelpMessage::HelpMessage(
    const Context *context, const Constraints *constraints, const size_t maxLineWidth
) : m_maxLineWidth(maxLineWidth), m_constraints(constraints) {
    appendHeader(context);
    appendBody(context, 0, FlagPath());
}

inline auto HelpMessage::get() const -> std::string {
    return m_message.str();
}

inline auto HelpMessage::indent(const size_t leadingSpaces) {
    for (size_t i = 0; i < leadingSpaces; i++) {
        m_message << ' ';
    }
}

// Prints the usage header and help messages for all options and positionals
inline auto HelpMessage::appendHeader(const Context *context) -> void {
    const auto inputHint = getContextInputHint(context, "[options]");
    m_message << "Usage:" << inputHint << "\n\n" << std::string(m_maxLineWidth, '-') << "\n";
}

// Gets the help message for all options and positionals
inline auto HelpMessage::appendBody( // NOLINT (misc-no-recursion)
    const Context *context, const size_t leadingSpaces, const FlagPath& groupPath
) -> void {
    bool sectionBeforeSet = appendOptions(context, leadingSpaces, groupPath);
    sectionBeforeSet |= appendPositionals(context, leadingSpaces, sectionBeforeSet, groupPath);
    appendGroups(context, leadingSpaces, sectionBeforeSet, groupPath);
}


inline auto HelpMessage::appendOptions(const Context *context, const size_t leadingSpaces, const FlagPath& groupPath) -> bool {
    const auto& options = context->getOptions();
    if (options.empty()) return false;
    indent(leadingSpaces);
    m_message << "Options:\n";
    for (const auto& holder : options) {
        appendIOption(leadingSpaces, holder.getPtr(), groupPath);
    }
    return true;
}

inline auto HelpMessage::appendPositionals(
    const Context *context, const size_t leadingSpaces, const bool printNewLine, const FlagPath& groupPath
) -> bool {
    const auto& positionals = context->getPositionals();
    if (positionals.empty()) return false;
    if (printNewLine) m_message << "\n";
    indent(leadingSpaces);
    m_message << "Positionals:\n";
    for (const auto& holder : positionals) {
        appendIOption(leadingSpaces, holder.getPtr(), groupPath);
    }
    return true;
}

inline auto HelpMessage::appendGroups( // NOLINT (misc-no-recursion)
    const Context *context, const size_t leadingSpaces, const bool printNewLine, const FlagPath& groupPath
) -> bool {
    const auto& groups = context->getGroups();
    if (groups.empty()) return false;
    if (printNewLine) m_message << "\n";
    indent(leadingSpaces);
    m_message << "Groups:\n";

    for (size_t i = 0; i < groups.size(); i++) {
        const auto& group = groups[i].getRef();
        appendIOption(leadingSpaces, &group, groupPath); m_message << "\n";
        indent(leadingSpaces + m_nextLevelIndent); m_message << getBasicInputHint(&group) << "\n";
        indent(leadingSpaces + m_nextLevelIndent); m_message << std::string(m_maxLineWidth - leadingSpaces -m_nextLevelIndent, '-') << "\n";
        auto newPath = groupPath;
        newPath.extendPath(group.getFlag().mainFlag);
        appendBody(&group.getContext(), leadingSpaces + 4, newPath);
        if (i + 1 < groups.size()) {
            m_message << "\n";
            indent(leadingSpaces + 2); m_message << std::string(m_maxLineWidth - leadingSpaces - 2, '-') << "\n";
        }
    }
    return true;
}

inline auto HelpMessage::appendName(const size_t leadingSpaces, const IOption *option) -> size_t {
    indent(leadingSpaces);
    const auto name = getOptionName(option);
    m_message << name;
    return name.length();
}

inline auto HelpMessage::appendInputHint(const size_t leadingSpaces, const size_t nameLength, const IOption *option) -> size_t {
    const auto inputHint = getFullInputHint(option);
    if (const auto inputSections = wrapString(inputHint, m_maxLineWidth - leadingSpaces);
        inputSections.empty()) {
        const auto width = static_cast<int>(m_maxFlagWidthBeforeWrapping - nameLength);
        m_message << std::left << std::setw(width) << ':';
    } else {
        for (size_t i = 0; i < inputSections.size(); i++) {
            const auto width = static_cast<int>(m_maxFlagWidthBeforeWrapping - nameLength);
            std::string buffer = " " + inputSections[i];
            buffer += i == inputSections.size() - 1 ? ':' : '\n';
            if (i != 0) {
                indent(leadingSpaces);
            }
            m_message << std::left << std::setw(width) << buffer;
        }
    }
    return inputHint.length();
}

inline auto HelpMessage::appendDescription(
    const size_t leadingSpaces, const IOption *option,
    const size_t nameLen, const size_t inputHintLen, FlagPath groupPath
) -> void {
    std::string description = option->getDescription();

    if (const auto flag = dynamic_cast<const IFlag *>(option); flag != nullptr) {
        groupPath.extendPath(flag->getFlag().mainFlag);
        if (ConstraintsQuery::containsRequirement(*m_constraints, groupPath)) {
            description += " (required)";
        }
        if (const auto opt = ConstraintsQuery::containsMutuallyExclusive(*m_constraints, groupPath)) {
            const auto& me = opt.value();
            description += " (mutually exclusive with: ";
            for (size_t i = 0; i < me.exclusives.size(); i++) {
                if (i != 0) {
                    description += ", ";
                }
                description += std::format(R"("{}")", me.exclusives[i].getString());
            }
            description += ")";
        }
        if (const auto opt = ConstraintsQuery::containsDependentOption(*m_constraints, groupPath)) {
            const auto& dep = opt.value();
            description += " (requires: ";
            for (size_t i = 0; i < dep.dependents.size(); i++) {
                if (i != 0) {
                    description += ", ";
                }
                description += std::format(R"("{}")", dep.dependents[i].getString());
            }
            description += ")";
        }
    }

    const size_t maxDescLength = m_maxLineWidth - m_maxFlagWidthBeforeWrapping - leadingSpaces;
    const std::vector<std::string> sections = wrapString(description, maxDescLength);

    constexpr size_t semicolonLen = 1;
    if (const auto flagLength = nameLen + inputHintLen + semicolonLen; flagLength > m_maxFlagWidthBeforeWrapping) {
        m_message << "\n" << std::string(leadingSpaces + m_maxFlagWidthBeforeWrapping, ' ') << sections[0];
    } else {
        m_message << sections[0];
    }
    m_message << "\n";

    for (size_t i = 1; i < sections.size(); i++) {
        if (sections[i].empty()) continue;
        m_message << std::string(leadingSpaces + m_maxFlagWidthBeforeWrapping, ' ') << sections[i] << "\n";
    }
}

// Gets a help message for an option/positional
inline auto HelpMessage::appendIOption(size_t leadingSpaces, const IOption *option, const FlagPath& groupPath) -> void {
    leadingSpaces += m_nameIndent;
    const size_t nameLen = appendName(leadingSpaces, option);
    const size_t inputHintLen = appendInputHint(leadingSpaces + nameLen, nameLen, option);
    appendDescription(leadingSpaces, option, nameLen, inputHintLen, groupPath);
}

}

#endif // ARGON_HELP_MESSAGE_INCLUDE
