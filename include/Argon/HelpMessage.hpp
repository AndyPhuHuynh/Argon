#ifndef ARGON_HELP_MESSAGE_INCLUDE
#define ARGON_HELP_MESSAGE_INCLUDE

#include <string>
#include <sstream>

#include "Argon/Context.hpp"

namespace Argon::detail {

class HelpMessage {
public:
    HelpMessage(const Context *context, size_t maxLineWidth);

    auto get() const -> std::string;
private:
    std::stringstream m_message;
    const size_t m_maxLineWidth;
    static constexpr size_t m_maxFlagWidthBeforeWrapping = 32;

    auto indent(size_t leadingSpaces);

    auto appendHeader(const Context *context) -> void;

    auto appendBody(const Context *context, size_t leadingSpaces) -> void;

    auto appendOptions(const Context *context, size_t leadingSpaces) -> bool;

    auto appendPositionals(const Context *context, size_t leadingSpaces, bool printNewLine) -> bool;

    auto appendGroups(const Context *context, size_t leadingSpaces, bool printNewLine) -> void;

    auto appendName(size_t leadingSpaces, const IOption *option) -> size_t;

    auto appendInputHint(size_t leadingSpaces, const IOption *option) -> size_t;

    auto appendIOption(size_t leadingSpaces, const IOption *option) -> void;
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

inline HelpMessage::HelpMessage(const Context *context, const size_t maxLineWidth) : m_maxLineWidth(maxLineWidth) {
    appendHeader(context);
    appendBody(context, 0);
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

    m_message << "Usage:" << inputHint << "\n\n"
        << "Options:\n" << std::string(m_maxLineWidth, '-') << "\n";
}

// Gets the help message for all options and positionals
inline auto HelpMessage::appendBody( // NOLINT (misc-no-recursion)
    const Context *context, const size_t leadingSpaces
) -> void {
    bool sectionBeforeSet = appendOptions(context, leadingSpaces);
    sectionBeforeSet |= appendPositionals(context, leadingSpaces, sectionBeforeSet);
    appendGroups(context, leadingSpaces, sectionBeforeSet);
}


inline auto HelpMessage::appendOptions(const Context *context, const size_t leadingSpaces) -> bool {
    const auto& options = context->getOptions();
    if (options.empty()) return false;
    indent(leadingSpaces);
    m_message << "Options:\n";
    for (const auto& holder : options) {
        appendIOption(leadingSpaces, holder.getPtr());
    }
    return true;
}

inline auto HelpMessage::appendPositionals(
    const Context *context, const size_t leadingSpaces, const bool printNewLine
) -> bool {
    const auto& positionals = context->getPositionals();
    if (positionals.empty()) return false;
    if (printNewLine) m_message << "\n";
    indent(leadingSpaces);
    m_message << "Positionals:\n";
    for (const auto& holder : positionals) {
        appendIOption(leadingSpaces, holder.getPtr());
    }
    return true;
}

inline auto HelpMessage::appendGroups( // NOLINT (misc-no-recursion)
    const Context *context, const size_t leadingSpaces, const bool printNewLine
) -> void {
    const auto& groups = context->getGroups();
    // Print group headers
    if (groups.empty()) return;
    if (printNewLine) m_message << "\n";
    auto leading = std::string(leadingSpaces, ' ');
    m_message << leading << "Groups:\n";
    leading += "    ";
    for (const auto& holder : groups) {
        const auto& group = holder.getRef();
        appendIOption(leadingSpaces, holder.getPtr());
        m_message << "\n" << leading;

        if (const auto& inputHint = group.getInputHint(); !inputHint.empty()) {
            m_message << inputHint;
        } else {
            m_message << std::format("[{}]", group.getFlag().getString());
        }

        m_message << "\n" << leading << std::string(m_maxLineWidth - leadingSpaces - 4, '-') << "\n";
        appendBody(&group.getContext(), leadingSpaces + 4);
    }
}

inline auto HelpMessage::appendName(const size_t leadingSpaces, const IOption *option) -> size_t {
    indent(leadingSpaces);
    const auto name = getOptionName(option);
    m_message << name;
    return name.length();
}

inline auto HelpMessage::appendInputHint(const size_t leadingSpaces, const IOption *option) -> size_t {
    const auto inputHint = getFullInputHint(option);
    if (const auto inputSections = wrapString(inputHint, m_maxLineWidth - leadingSpaces);
        inputSections.empty()) {
        const auto width = static_cast<int>(m_maxFlagWidthBeforeWrapping - leadingSpaces);
        m_message << std::left << std::setw(width) << ':';
    } else {
        for (size_t i = 0; i < inputSections.size(); i++) {
            const auto width = static_cast<int>(m_maxFlagWidthBeforeWrapping - leadingSpaces);
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

// Gets a help message for an option/positional
inline auto HelpMessage::appendIOption(const size_t leadingSpaces, const IOption *option) -> void {
    constexpr size_t extraNameIndent = 2;
    const size_t nameLen = appendName(leadingSpaces + extraNameIndent, option);
    const size_t inputHintLen = appendInputHint(leadingSpaces + nameLen + extraNameIndent, option);

    // Print description with line wrapping
    const std::string& description = option->getDescription();

    const size_t maxDescLength = m_maxLineWidth - m_maxFlagWidthBeforeWrapping;
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

}

#endif // ARGON_HELP_MESSAGE_INCLUDE
