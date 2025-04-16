﻿#include "testing.hpp"

#include <vector>

#include <Argon/Scanner.hpp>

using namespace Argon;

static void Test1() {
    std::string str = "hello world! [inside brackets]";
    Scanner scanner(str);

    Token token = scanner.getNextToken();
    std::vector<Token> tokens;

    while (token.kind != TokenKind::END) {
        tokens.push_back(token);
        token = scanner.getNextToken();
    }

    std::vector<Token> expected = {
        {TokenKind::IDENTIFIER, "hello"},
        {TokenKind::IDENTIFIER, "world!"},
        {TokenKind::LBRACK},
        {TokenKind::IDENTIFIER, "inside"},
        {TokenKind::IDENTIFIER, "brackets"},
        {TokenKind::RBRACK},
    };

    bool equals = true;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] != expected[i]) {
            equals = false;
            break;
        }
    }
    equals &= tokens.size() == expected.size();
    check(equals, "Scanner Test 1");
}

static void Test2() {
    std::string str = "hello world!                  [[[[inside brackets]";
    Scanner scanner(str);

    Token token = scanner.getNextToken();
    std::vector<Token> tokens;

    while (token.kind != TokenKind::END) {
        tokens.push_back(token);
        token = scanner.getNextToken();
    }

    std::vector<Token> expected = {
        {TokenKind::IDENTIFIER, "hello"},
        {TokenKind::IDENTIFIER, "world!"},
        {TokenKind::LBRACK},
        {TokenKind::LBRACK},   
        {TokenKind::LBRACK},   
        {TokenKind::LBRACK},   
        {TokenKind::IDENTIFIER, "inside"},
        {TokenKind::IDENTIFIER, "brackets"},
        {TokenKind::RBRACK},
    };

    bool equals = true;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] != expected[i]) {
            equals = false;
            break;
        }
    }
    equals &= tokens.size() == expected.size();
    check(equals, "Scanner Test 2");
}

void runScannerTests() {
    Test1();
    Test2();
}