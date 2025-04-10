﻿#pragma once

#include <string>
#include <vector>

namespace Argon {
    enum TokenKind : uint8_t {
        NONE = 0,
        LBRACK,
        RBRACK,
        IDENTIFIER,
        END,
    };

    struct Token {
        TokenKind kind;
        std::string image;

        Token() = default;
        Token(TokenKind kind);
        Token(TokenKind kind, std::string image);
        bool operator==(std::vector<Token>::const_reference token) const;
    };

    std::string getDefaultImage(TokenKind kind);
    
    class Scanner {
        std::string m_buffer;
        uint32_t m_bufferPos = 0;
        bool m_readFromSee;
        Token m_seeToken;
    public:
        Scanner() = default;
        Scanner(std::string buffer) : m_buffer(std::move(buffer)) {}

        bool seeTokenKind(TokenKind kind);
        bool haveTokenKind(Token token);
        char peekChar();
        char nextChar();
        Token getNextToken();
    };
}
