#pragma once

#include <cstdint>
#include <string>
#include <string_view>
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
        TokenKind kind = TokenKind::NONE;
        std::string image;
        int position = -1;

        Token() = default;
        explicit Token(TokenKind kind);
        Token(TokenKind kind, std::string image);
        Token(TokenKind kind, int position);
        Token(TokenKind kind, std::string image, int position);
        bool operator==(std::vector<Token>::const_reference token) const;
    };

    std::string getDefaultImage(TokenKind kind);
    bool isOneOf(const Token& token, const std::initializer_list<TokenKind>& kinds);
    
    class Scanner {
    public:
        Scanner() = default;
        explicit Scanner(std::string_view buffer);

        [[nodiscard]] bool seeTokenKind(TokenKind kind) const;
        [[nodiscard]] bool seeTokenKind(const std::initializer_list<TokenKind>& kinds) const;
        [[nodiscard]] char peekChar() const;
        char nextChar();
        Token getNextToken();
        [[nodiscard]] Token peekToken() const;
        
        void scanUntilSee(const std::initializer_list<TokenKind>& kinds);
        Token scanUntilGet(const std::initializer_list<TokenKind>& kinds);
        
        void recordPosition();
        void rewind();
    private:
        std::vector<Token> m_tokens;
        uint32_t m_tokenPos = 0;
        uint32_t m_rewindPos = 0;

        std::string_view m_buffer;
        uint32_t m_bufferPos = 0;

        void scanNextToken();
        void scanBuffer();
    };
}
