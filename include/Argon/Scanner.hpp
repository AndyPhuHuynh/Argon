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
        [[nodiscard]] bool isOneOf(const std::initializer_list<TokenKind>& kinds) const;
    };

    std::string getDefaultImage(TokenKind kind);
    
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

// --------------------------------------------- Implementations -------------------------------------------------------

#include <algorithm>
#include <sstream>
#include <unordered_map>

inline Argon::Token::Token(const TokenKind kind) : kind(kind) {
    image = getDefaultImage(kind);
}

inline Argon::Token::Token(const TokenKind kind, std::string image) : kind(kind), image(std::move(image)) {}

inline Argon::Token::Token(const TokenKind kind, const int position) : kind(kind), position(position) {
    image = getDefaultImage(kind);
}

inline Argon::Token::Token(const TokenKind kind, std::string image, const int position)
    : kind(kind), image(std::move(image)), position(position) {}

inline bool Argon::Token::operator==(std::vector<Token>::const_reference token) const {
    return kind == token.kind && image == token.image;
}

inline bool Argon::Token::isOneOf(const std::initializer_list<TokenKind> &kinds) const {
    return std::ranges::contains(kinds, kind);
}

inline std::string Argon::getDefaultImage(const TokenKind kind) {
    static const std::unordered_map<TokenKind, std::string> kindToImage = {
        { TokenKind::LBRACK, "[" },
        { TokenKind::RBRACK, "]" },
        { TokenKind::END, "" }
    };

    return kindToImage.contains(kind) ? kindToImage.at(kind) : "";
}

inline Argon::Scanner::Scanner(const std::string_view buffer) {
    m_buffer = buffer;
    scanBuffer();
}

inline bool Argon::Scanner::seeTokenKind(const TokenKind kind) const {
    return peekToken().kind == kind;
}

inline bool Argon::Scanner::seeTokenKind(const std::initializer_list<TokenKind>& kinds) const {
    return std::ranges::contains(kinds, peekToken().kind);
}

inline char Argon::Scanner::peekChar() const {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos];
}

inline char Argon::Scanner::nextChar() {
    if (m_bufferPos >= m_buffer.size()) {
        return EOF;
    }
    return m_buffer[m_bufferPos++];
}

inline Argon::Token Argon::Scanner::peekToken() const {
    if (m_tokenPos >= m_tokens.size()) {
        return {TokenKind::END, static_cast<int>(m_buffer.size())};
    } else {
        return m_tokens[m_tokenPos];
    }
}

inline Argon::Token Argon::Scanner::getNextToken() {
    if (m_tokenPos >= m_tokens.size()) {
        return {TokenKind::END, static_cast<int>(m_buffer.size())};
    } else {
        return m_tokens[m_tokenPos++];
    }
}

inline void Argon::Scanner::scanUntilSee(const std::initializer_list<TokenKind>& kinds) {
    while (!seeTokenKind(kinds) && !seeTokenKind(TokenKind::END)) {
        getNextToken();
    }
}

inline Argon::Token Argon::Scanner::scanUntilGet(const std::initializer_list<TokenKind>& kinds) {
    scanUntilSee(kinds);
    return getNextToken();
}


inline void Argon::Scanner::recordPosition() {
    m_rewindPos = m_tokenPos;
}

inline void Argon::Scanner::rewind() {
    m_tokenPos = m_rewindPos;
}

inline void Argon::Scanner::scanNextToken() {
    int position = static_cast<int>(m_bufferPos);
    char ch = nextChar();
    while (true) {
        if (ch == ' ') {
            ch = nextChar();
            continue;
        }
        else if (ch == EOF) {
            m_tokens.emplace_back(TokenKind::END, position);
            return;
        } else if (ch == '[') {
            m_tokens.emplace_back(TokenKind::LBRACK, position);
            return;
        } else if (ch == ']') {
            m_tokens.emplace_back(TokenKind::RBRACK, position);
            return;
        }
        // Token is identifier
        else {
            std::stringstream ss;
            ss << ch;
            ch = peekChar();
            while (ch != ' ' && ch != EOF && ch != '[' && ch != ']') {
                ch = nextChar();
                ss << ch;
                ch = peekChar();
            }
            m_tokens.emplace_back(TokenKind::IDENTIFIER, ss.str(), position);
            return;
        }
    }
}

inline void Argon::Scanner::scanBuffer() {
    while (m_bufferPos < m_buffer.size() ||  m_tokens.back().kind != TokenKind::END) {
        scanNextToken();
    }
}

