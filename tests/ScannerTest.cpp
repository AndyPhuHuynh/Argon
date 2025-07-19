#include <vector>

#include "catch2/catch_test_macros.hpp"

#include <Argon/Scanner.hpp>
using namespace Argon;

TEST_CASE("Scanner test 1", "[scanner]") {
    const std::string str = "hello world! [inside brackets]";
    const Scanner scanner(str);
    const auto& tokens = scanner.getAllTokens();

    const std::vector<Token> expected = {
        {TokenKind::IDENTIFIER, "hello",     0},
        {TokenKind::IDENTIFIER, "world!",    6},
        {TokenKind::LBRACK,     "[",        13},
        {TokenKind::IDENTIFIER, "inside",   14},
        {TokenKind::IDENTIFIER, "brackets", 21},
        {TokenKind::RBRACK,     "]",        29},
        {TokenKind::END,        "",         30}
    };

    REQUIRE(tokens.size() == expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        INFO("Mismatched token at index: " << i);
        REQUIRE(tokens[i] == expected[i]);
    }
}

TEST_CASE("Scanner ignores whitespaces") {
    const std::string str = "hello            world!                  [[        [[inside        brackets]";
    const Scanner scanner(str);
    const auto& tokens = scanner.getAllTokens();

    const std::vector<Token> expected = {
        {TokenKind::IDENTIFIER, "hello",     0},
        {TokenKind::IDENTIFIER, "world!",   17},
        {TokenKind::LBRACK,     "[",        41},
        {TokenKind::LBRACK,     "[",        42},
        {TokenKind::LBRACK,     "[",        51},
        {TokenKind::LBRACK,     "[",        52},
        {TokenKind::IDENTIFIER, "inside",   53},
        {TokenKind::IDENTIFIER, "brackets", 67},
        {TokenKind::RBRACK,     "]",        75},
        {TokenKind::END,        "",         76}
    };

    REQUIRE(tokens.size() == expected.size());

    for (size_t i = 0; i < tokens.size(); i++) {
        INFO("Mismatched token at index: " << i);
        REQUIRE(tokens[i] == expected[i]);
    }
}