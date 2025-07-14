#ifndef ARGON_HELP_MESSAGE_INCLUDE
#define ARGON_HELP_MESSAGE_INCLUDE

#include <string>
#include <sstream>

#include "Argon/Context.hpp"

namespace Argon::detail {

// Gets a help message for an option/positional
inline auto getHelpMessageForIOption(
    std::stringstream& ss, size_t leadingSpaces, const size_t maxLineWidth, const IOption *option,
    const PositionalPolicy defaultPolicy
) -> void {
    leadingSpaces += 2;
    constexpr size_t maxFlagWidthBeforeWrapping = 32;

    // Print name
    const auto namePadding = std::string(leadingSpaces, ' ');
    const auto name = getOptionName(option);
    const auto inputPadding = std::string(leadingSpaces + name.length(), ' ');
    const auto inputHint = getFullInputHint(option, defaultPolicy);
    const auto inputSections = wrapString(inputHint, maxLineWidth - inputPadding.length());
    ss << namePadding << name;
    if (inputSections.empty()) {
        const auto width = static_cast<int>(maxFlagWidthBeforeWrapping - name.length());
        ss << std::left << std::setw(width) << ':';
    } else {
        for (size_t i = 0; i < inputSections.size(); i++) {
            const auto width = static_cast<int>(maxFlagWidthBeforeWrapping - name.length());
            std::string buffer = " " + inputSections[i];
            buffer += i == inputSections.size() - 1 ? ':' : '\n';
            if (i != 0) {
                ss << inputPadding;
            }
            ss << std::left << std::setw(width) << buffer;
        }
    }

    // Print description with line wrapping
    const std::string& description = option->getDescription();

    const size_t maxDescLength = maxLineWidth - maxFlagWidthBeforeWrapping;
    const std::vector<std::string> sections = wrapString(description, maxDescLength);

    // +1 in flag length is for the semicolon
    if (const auto flagLength = name.length() + inputHint.length() + 1; flagLength > maxFlagWidthBeforeWrapping) {
        ss << "\n" << std::string(leadingSpaces + maxFlagWidthBeforeWrapping, ' ') << sections[0];
    } else {
        ss << sections[0];
    }
    ss << "\n";

    for (size_t i = 1; i < sections.size(); i++) {
        if (sections[i].empty()) continue;
        ss << std::string(leadingSpaces + maxFlagWidthBeforeWrapping, ' ') << sections[i] << "\n";
    }
}


// Get help message for the Options vector of a context
inline auto getHelpMessageForOptions(
    const Context *context, std::stringstream& ss, const size_t leadingSpaces, const size_t maxLineWidth,
    std::vector<const OptionGroup *>& groups, bool& sectionBeforeSet
) -> void {
    const auto& options = context->getOptions();
    if (options.empty()) return;
    for (size_t i = 0; i < leadingSpaces; i++)
        ss << ' ';
    ss << "Options:\n";

    sectionBeforeSet = true;
    for (const auto& holder : options) {
        const auto optPtr = holder.getPtr();
        assertHasFlag(optPtr);
        if (const auto groupPtr = dynamic_cast<const OptionGroup*>(optPtr); groupPtr != nullptr) {
            groups.push_back(groupPtr);
            continue;
        }
        getHelpMessageForIOption(ss, leadingSpaces, maxLineWidth, optPtr, context->getDefaultPositionalPolicy());
    }
}

// Gets the help message for all options and positionals
inline auto getHelpMessage( // NOLINT (misc-no-recursion)
    const Context *context, std::stringstream& ss, const size_t leadingSpaces,
    const size_t maxLineWidth, const PositionalPolicy defaultPolicy
) -> void {
    const auto& m_positionals = context->getPositionals();

    std::vector<const OptionGroup*> groups;
    auto leading = std::string(leadingSpaces, ' ');
    bool sectionBeforeSet = false;

    getHelpMessageForOptions(context, ss, leadingSpaces, maxLineWidth, groups, sectionBeforeSet);

    if (!m_positionals.empty()) {
        if (sectionBeforeSet) ss << "\n";
        ss << leading << "Positionals:\n";
        sectionBeforeSet = true;
    }
    for (const auto& holder : m_positionals) {
        getHelpMessageForIOption(ss, leadingSpaces, maxLineWidth, holder.getPtr(), defaultPolicy);
    }

    // Print group messages
    if (!groups.empty()) {
        if (sectionBeforeSet) ss << "\n";
        ss << leading << "Groups:\n";
        leading += "    ";
    }
    for (const auto& group : groups) {
        getHelpMessageForIOption(ss, leadingSpaces, maxLineWidth, group, defaultPolicy);
        ss << "\n" << leading;

        if (const auto& inputHint = group->getInputHint(); !inputHint.empty()) {
            ss << inputHint;
        } else {
            ss << std::format("[{}]", group->getFlag().getString());
        }

        ss << "\n" << leading << std::string(maxLineWidth - leadingSpaces - 4, '-') << "\n";
        getHelpMessage(&group->getContext(), ss, leadingSpaces + 4, maxLineWidth, defaultPolicy);
    }
}

// Prints the usage header and help messages for all options and positionals
inline auto getHelpMessage(const Context *context, const size_t maxLineWidth) -> std::string {
    std::stringstream ss;
    ss << "Usage: [options]\n\n" << "Options:\n" << std::string(maxLineWidth, '-') << "\n";
    getHelpMessage(context, ss, 0, maxLineWidth, context->getDefaultPositionalPolicy());
    return ss.str();
}

}

#endif // ARGON_HELP_MESSAGE_INCLUDE
