#ifndef ARGON_ERROR_TEST_FUNCTIONS_INCLUDE
#define ARGON_ERROR_TEST_FUNCTIONS_INCLUDE

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

#include "Argon/Error.hpp"

inline auto RequireMsg(const Argon::ErrorVariant& var) {
    REQUIRE(std::holds_alternative<Argon::ErrorMessage>(var));
    return std::get<Argon::ErrorMessage>(var);
}

inline auto RequireGroup(const Argon::ErrorVariant& var) {
    REQUIRE(std::holds_alternative<Argon::ErrorGroup>(var));
    return std::get<Argon::ErrorGroup>(var);
}

inline auto CheckMessage(const Argon::ErrorMessage& error, const std::string_view msg, const int pos, Argon::ErrorType type) {
    CAPTURE(msg, pos, type);
    CAPTURE(error.msg, error.pos, error.type);
    CHECK(error.msg == msg);
    CHECK(error.pos == pos);
    CHECK(error.type == type);
}

inline auto CheckMessage(const Argon::ErrorMessage& error, const int pos, Argon::ErrorType type) {
    CAPTURE(pos, type);
    CAPTURE(error.msg, error.pos, error.type);
    CHECK(error.pos == pos);
    CHECK(error.type == type);
}

inline auto CheckMessage(const Argon::ErrorMessage& error, const std::initializer_list<std::string_view> expectedMsgs,
                         const int pos, Argon::ErrorType type) {
    CAPTURE(expectedMsgs, pos, type);
    CAPTURE(error.msg, error.pos, error.type);
    CHECK(error.pos == pos);
    CHECK(error.type == type);

    for (const auto& msg : expectedMsgs) {
        CHECK_THAT(error.msg, Catch::Matchers::ContainsSubstring(std::string(msg)));
    }
}


inline auto CheckGroup(const Argon::ErrorGroup& group, const std::string_view groupName,
    const int start, const int end, const size_t errorCount) -> const Argon::ErrorGroup& {
    CAPTURE(groupName, start, end, errorCount);
    CAPTURE(group.getGroupName(), group.getStartPosition(), group.getEndPosition());

    CHECK(group.getGroupName() == groupName);
    CHECK(group.getStartPosition() == start);
    CHECK(group.getEndPosition() == end);

    const auto& errors = group.getErrors();
    CAPTURE(errors.size());
    REQUIRE(errors.size() == errorCount);

    return group;
}

inline auto DigitToString(const int i) {
    if (i == 0) return "zero";
    if (i == 1) return "one";
    if (i == 2) return "two";
    if (i == 3) return "three";
    if (i == 4) return "four";
    if (i == 5) return "five";
    if (i == 6) return "six";
    if (i == 7) return "seven";
    if (i == 8) return "eight";
    if (i == 9) return "nine";
    return "none";
}
#endif // ARGON_ERROR_TEST_FUNCTIONS_INCLUDE
