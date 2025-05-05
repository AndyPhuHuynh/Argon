#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Argon {
    enum class TokenKind : uint8_t {
        NONE = 0,
        LBRACK,
        RBRACK,
        IDENTIFIER,
        END,
    };

    struct Token {
        TokenKind kind;
        std::string image;
        int position = 0;

        Token() = default;
        Token(TokenKind kind);
        Token(TokenKind kind, std::string image);
        Token(TokenKind kind, int position);
        Token(TokenKind kind, std::string image, int position);
        bool operator==(std::vector<Token>::const_reference token) const;
    };

    std::string getDefaultImage(TokenKind kind);
    
    class Scanner {
        std::string m_buffer;
        uint32_t m_bufferPos = 0;
        bool m_readFromSee;
        Token m_seeToken;

        uint32_t m_rewindPos = 0;
    public:
        Scanner() = default;
        Scanner(std::string buffer) : m_buffer(std::move(buffer)) {}

        bool seeTokenKind(TokenKind kind);
        bool haveTokenKind(Token token);
        char peekChar();
        char nextChar();
        Token getNextToken();
        
        void scanUntilSee(const std::initializer_list<TokenKind>& kinds);
        Token scanUntilGet(const std::initializer_list<TokenKind>& kinds);
        
        void recordPosition();
        void rewind();
    };
}
